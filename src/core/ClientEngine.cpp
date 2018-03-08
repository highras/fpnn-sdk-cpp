#include <sys/select.h>
//#include <sys/sysinfo.h>
//#include <sys/types.h>
//#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <atomic>
#include <list>
#include "Config.h"
#include "FPLog.h"
#include "ignoreSignals.h"
#include "NetworkUtility.h"
#include "TimeUtil.h"
#include "TCPClient.h"
#include "ClientEngine.h"

using namespace fpnn;

std::mutex ClientEngine::_mutex;
static std::atomic<bool> _created(false);
static ClientEnginePtr _engine;

ClientEnginePtr ClientEngine::create(const ClientEngineInitParams *params)
{
	if (!_created)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (!_created)
		{
			_engine.reset(new ClientEngine(params));
			_created = true;
		}
	}
	return _engine;
}

ClientEngine::ClientEngine(const ClientEngineInitParams *params): _running(true),
	_newSocketSetChanged(false), _waitWriteSetChanged(false), _quitSocketSetChanged(false)
{
	ClientEngineInitParams defaultParams;
	if (!params)
		params = &defaultParams;

	if (params->ignoreSignals)
		ignoreSignals();

	_logHolder = FPLog::instance();

	_questTimeout = params->globalTimeoutSeconds * 1000;

	if (pipe(_notifyFds) != 0)	//-- Will failed when current processor using to many fds, or the system limitation reached.
		LOG_FATAL("ClientEngine create pipe for notification failed.");

	nonblockedFd(_notifyFds[0]);
	nonblockedFd(_notifyFds[1]);

	_callbackPool.init(0, 1, params->residentTaskThread, params->maxTaskThreads);

	_loopThread = std::thread(&ClientEngine::loopThread, this);
	_timeoutChecker = std::thread(&ClientEngine::timeoutCheckThread, this);
}

ClientEngine::~ClientEngine()
{
	_running = false;

	int count = (int)write(_notifyFds[1], this, 4);
	(void)count;

	_timeoutChecker.join();
	_loopThread.join();

	close(_notifyFds[1]);
	close(_notifyFds[0]);
}

bool ClientEngine::join(const TCPClientConnection* connection)
{
	int socket = connection->socket();
	if (!nonblockedFd(socket))
	{
		LOG_ERROR("Change socket to non-blocked failed. %s", connection->_connectionInfo->str().c_str());
		return false;
	}
	_connectionMap.insert(socket, (TCPClientConnection*)connection);

	bool rev = true;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (_newSocketSet.size() < FD_SETSIZE)
		{
			_newSocketSet.insert(socket);
			_newSocketSetChanged = true;
		}
		else
			rev = false;
	}

	if (rev)
	{
		int count = (int)write(_notifyFds[1], this, 4);
		(void)count;
	}
	else
		_connectionMap.remove(socket);

	return rev;
}

bool ClientEngine::waitSendEvent(const TCPClientConnection* connection)
{
	{
		std::unique_lock<std::mutex> lck(_mutex);
		_waitWriteSet.insert(connection->socket());
		_waitWriteSetChanged = true;
	}

	int count = (int)write(_notifyFds[1], this, 4);
	(void)count;
	return true;
}

void ClientEngine::quit(const TCPClientConnection* connection)
{
	int socket = connection->socket();
	_connectionMap.remove(socket);

	{
		std::unique_lock<std::mutex> lck(_mutex);
		_quitSocketSet.insert(socket);
		_quitSocketSetChanged = true;
	}

	int count = (int)write(_notifyFds[1], this, 4);
	(void)count;
}

void ClientEngine::sendData(int socket, uint64_t token, std::string* data)
{
	if (!_connectionMap.sendData(socket, token, data))
	{
		delete data;
		LOG_ERROR("Data not send at socket %d. socket maybe closed.", socket);
	}
}

void ClientEngine::clearConnectionQuestCallbacks(TCPClientConnection* connection, int errorCode)
{
	for (auto callbackPair: connection->_callbackMap)
	{
		BasicAnswerCallback* callback = callbackPair.second;
		if (callback->syncedCallback())		//-- check first, then fill result.
			callback->fillResult(NULL, errorCode);
		else
		{
			callback->fillResult(NULL, errorCode);

			BasicAnswerCallbackPtr task(callback);
			_callbackPool.wakeUp(task);
		}
	}
	// connection->_callbackMap.clear(); //-- If necessary.
}

void ClientEngine::clearConnection(int socket, int errorCode)
{
	TCPClientConnection* conn = _connectionMap.takeConnection(socket);
	if (conn == NULL)
		return;

	_connectionMap.remove(socket);
	clearConnectionQuestCallbacks(conn, errorCode);
	
	TCPClientPtr client = conn->client();
	if (client)
		client->willClosed(conn, true);
	else
	{
		CloseErrorTaskPtr task(new CloseErrorTask(conn, true));
		_callbackPool.wakeUp(task);
		reclaim(task);
	}
}

void ClientEngine::clean()
{
	std::set<int> fdSet;
	_connectionMap.getAllSocket(fdSet);
	
	for (int socket: fdSet)
		clearConnection(socket, FPNN_EC_CORE_CONNECTION_CLOSED);

	_connectionMap.waitForEmpty();
}

void ClientEngine::processConnectionIO(int fd, bool canRead, bool canWrite)
{
	TCPClientConnection* conn = _connectionMap.signConnection(fd);
	if (!conn)
		return;

	TCPClientIOProcessor::processConnectionIO(conn, canRead, canWrite);
}

void ClientEngine::loopThread()
{
	fd_set rfds;
	fd_set wfds;
	fd_set efds;

	std::set<int> allSocket; //-- except _notifyFds[0].
	std::set<int> wantWriteSocket;

	struct ConnStatInfo {
		bool canRead;
		bool canWrite;

		ConnStatInfo(): canRead(false), canWrite(false) {}
	};

	while (_running)
	{
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&efds);

		int maxfd = _notifyFds[0];

		FD_SET(_notifyFds[0], &rfds);
		for (int socket: allSocket)
		{
			FD_SET(socket, &rfds);
			FD_SET(socket, &efds);

			if (socket > maxfd)
				maxfd = socket;
		}

		for (int socket: wantWriteSocket)
			FD_SET(socket, &wfds);

		
		int activeCount = select(maxfd + 1, &rfds, &wfds, &efds, NULL);
		if (activeCount > 0)
		{
			if (FD_ISSET(_notifyFds[0], &rfds))		//-- MUST do this before check _running flag.
				consumeNotifyData();

			if (_running == false)
				break;

			std::map<int, ConnStatInfo> connStatus;
			//-- error & read set
			{
				std::set<int> dropped;
				for (int socket: allSocket)
				{
					if (FD_ISSET(socket, &efds))
					{
						FD_CLR(socket, &wfds);

						dropped.insert(socket);
						clearConnection(socket, FPNN_EC_CORE_UNKNOWN_ERROR);
					}
					else if (FD_ISSET(socket, &rfds))
						connStatus[socket].canRead = true;
				}

				for (int socket: dropped)
				{
					allSocket.erase(socket);
					wantWriteSocket.erase(socket);
				}
			}

			//-- write set
			{
				for (int socket: wantWriteSocket)
					if (FD_ISSET(socket, &wfds))
						connStatus[socket].canWrite = true;
			}

			for (auto& csp: connStatus)
				processConnectionIO(csp.first, csp.second.canRead, csp.second.canWrite);

			//-- check event flags
			{
				std::unique_lock<std::mutex> lck(_mutex);
				if (_quitSocketSetChanged)
				{
					for (int socket: _quitSocketSet)
					{
						allSocket.erase(socket);
						wantWriteSocket.erase(socket);
					}
					_quitSocketSet.clear();
					_quitSocketSetChanged = false;
				}

				if (_newSocketSetChanged)
				{
					for (int socket: _newSocketSet)
						allSocket.insert(socket);
					
					_newSocketSet.clear();
					_newSocketSetChanged = false;
				}

				if (_waitWriteSetChanged)
				{
					for (int socket: _waitWriteSet)
						wantWriteSocket.insert(socket);
					
					_waitWriteSet.clear();
					_waitWriteSetChanged = false;
				}
			}
		}
		else if (activeCount == -1)
		{
			LOG_ERROR("Unknown Error when select() errno: %d", errno);
			break;
		}
	}

	clean();
}

void ClientEngine::consumeNotifyData()
{
	const int buf_len = 4;

	int fd = _notifyFds[0];
	char buf[buf_len];

	while (true)
	{
		int readBytes = (int)::read(fd, buf, buf_len);
		if (readBytes != buf_len)
		{
			if (errno == EINTR || (readBytes > 0 && errno == 0))
				continue;
			else
				return;
		}
	}
}

void ClientEngine::reclaimConnections()
{
	std::set<IReleaseablePtr> deleted;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		for (IReleaseablePtr object: _reclaimedConnections)
		{
			if (object->releaseable())
				deleted.insert(object);
		}
		for (IReleaseablePtr object: deleted)
			_reclaimedConnections.erase(object);
	}
	deleted.clear();
}

void ClientEngine::clearTimeoutQuest()
{
	int64_t current = TimeUtil::curr_msec();
	std::list<std::map<uint32_t, BasicAnswerCallback*> > timeouted;

	_connectionMap.extractTimeoutedCallback(current, timeouted);
	for (auto bacMap: timeouted)
	{
		for (auto bacPair: bacMap)
		{
			if (bacPair.second)
			{
				BasicAnswerCallback* callback = bacPair.second;
				if (callback->syncedCallback())		//-- check first, then fill result.
					callback->fillResult(NULL, FPNN_EC_CORE_TIMEOUT);
				else
				{
					callback->fillResult(NULL, FPNN_EC_CORE_TIMEOUT);

					BasicAnswerCallbackPtr task(callback);
					_callbackPool.wakeUp(task);
				}
			}
		}
	}
}

void ClientEngine::timeoutCheckThread()
{
	while (_running)
	{
		int cyc = 100;
		while (_running && cyc--)
			usleep(10000);

		clearTimeoutQuest();
		reclaimConnections();
	}
}

void CloseErrorTask::run()
{
	if (_connection->_questProcessor == NULL)
		return;

	try
	{
		_connection->_questProcessor->connectionWillClose(*(_connection->_connectionInfo), _error);
	}
	catch (const FpnnError& ex){
		LOG_ERROR("CloseErrorTask::run() function.(%d)%s, connection:%s", ex.code(), ex.what(), _connection->_connectionInfo->str().c_str());
	}
	catch (...)
	{
		LOG_ERROR("Unknown error when calling CloseErrorTask::run() function. %s", _connection->_connectionInfo->str().c_str());
	}
}
