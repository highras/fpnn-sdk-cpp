#include <exception>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
//#include <errno.h>
#include "Config.h"
#include "ClientEngine.h"
#include "FPWriter.h"
#include "AutoRelease.h"
#include "FileSystemUtil.h"
#include "PEM_DER_SAX.h"
#include "TCPClient.h"

using namespace fpnn;

const char* TCPClient::SDKVersion = "0.1.2";

TCPClient::TCPClient(const std::string& host, int port, bool autoReconnect): _connected(false),
	_connStatus(ConnStatus::NoConnected), _timeoutQuest(0),
	_AESKeyLen(16), _packageEncryptionMode(true), _autoReconnect(autoReconnect)
{
	_engine = ClientEngine::instance();

	if (host.find(':') == std::string::npos)
	{
		if (checkIP4(host))
		{
			_isIPv4 = true;
			_connectionInfo.reset(new ConnectionInfo(0, port, host, true));
			_endpoint = std::string(host + ":").append(std::to_string(port));
		}
		else
		{
			std::string IPAddress;
			EndPointType eType;
			if (getIPAddress(host, IPAddress, eType))
			{
				if (eType == ENDPOINT_TYPE_IP4)
				{
					_isIPv4 = true;
					_connectionInfo.reset(new ConnectionInfo(0, port, IPAddress, true));
					_endpoint = std::string(IPAddress + ":").append(std::to_string(port));
				}
				else
				{
					_isIPv4 = false;
					_connectionInfo.reset(new ConnectionInfo(0, port, IPAddress, false));
					_endpoint = std::string("[").append(IPAddress).append("]:").append(std::to_string(port));
				}
			}
			else
			{
				LOG_ERROR("Get IP address for %s failed. Current client is invalid.", host.c_str());

				//-- for other logic can report error in right way, we still treat the invalid host as IPv4 address.
				_isIPv4 = true;
				_connectionInfo.reset(new ConnectionInfo(0, port, host, true));
				_endpoint = std::string(host + ":").append(std::to_string(port));
			}
		}
	}
	else
	{
		_isIPv4 = false;
		_connectionInfo.reset(new ConnectionInfo(0, port, host, false));
		_endpoint = std::string("[").append(host).append("]:").append(std::to_string(port));
	}
}

TCPClient::~TCPClient()
{
	if (_connected)
	{
		_autoReconnect = false;
		close();
	}
}

bool TCPClient::enableEncryptorByDerData(const std::string &derData, bool packageMode, bool reinforce)
{
	EccKeyReader reader;

	X690SAX derSAX;
	if (derSAX.parse(derData, &reader) == false)
		return false;

	enableEncryptor(reader.curveName(), reader.rawPublicKey(), packageMode, reinforce);
	return true;
}
bool TCPClient::enableEncryptorByPemData(const std::string &PemData, bool packageMode, bool reinforce)
{
	EccKeyReader reader;

	PemSAX pemSAX;
	if (pemSAX.parse(PemData, &reader) == false)
		return false;

	enableEncryptor(reader.curveName(), reader.rawPublicKey(), packageMode, reinforce);
	return true;
}
bool TCPClient::enableEncryptorByDerFile(const char *derFilePath, bool packageMode, bool reinforce)
{
	std::string content;
	if (FileSystemUtil::readFileContent(derFilePath, content) == false)
		return false;
	
	return enableEncryptorByDerData(content, packageMode, reinforce);
}
bool TCPClient::enableEncryptorByPemFile(const char *pemFilePath, bool packageMode, bool reinforce)
{
	std::string content;
	if (FileSystemUtil::readFileContent(pemFilePath, content) == false)
		return false;

	return enableEncryptorByPemData(content, packageMode, reinforce);
}

void TCPClient::connected(TCPClientConnection* connection)
{
	if (_questProcessor)
	{
		try
		{
			_questProcessor->connected(*(connection->_connectionInfo));
		}
		catch (const FpnnError& ex){
			LOG_ERROR("connected() error:(%d)%s. %s", ex.code(), ex.what(), connection->_connectionInfo->str().c_str());
		}
		catch (...)
		{
			LOG_ERROR("Unknown error when calling connected() function. %s", connection->_connectionInfo->str().c_str());
		}
	}
}

class ErrorCloseTask: virtual public ITaskThreadPool::ITask, virtual public IReleaseable
{
	bool _error;
	bool _executed;
	TCPClientConnection* _connection;
	IQuestProcessorPtr _questProcessor;

public:
	ErrorCloseTask(IQuestProcessorPtr questProcessor, TCPClientConnection* connection, bool error):
		_error(error), _executed(false), _connection(connection), _questProcessor(questProcessor) {}

	virtual ~ErrorCloseTask()
	{
		if (!_executed)
			run();

		delete _connection;
	}

	bool releaseable()
	{
		return (_connection->_refCount == 0);
	}

	virtual void run()
	{
		_executed = true;

		if (_questProcessor)
		try
		{
			_questProcessor->connectionWillClose(*(_connection->_connectionInfo), _error);
		}
		catch (const FpnnError& ex){
			LOG_ERROR("ErrorCloseTask::run() error:(%d)%s. %s", ex.code(), ex.what(), _connection->_connectionInfo->str().c_str());
		}
		catch (...)
		{
			LOG_ERROR("Unknown error when calling ErrorCloseTask::run() function. %s", _connection->_connectionInfo->str().c_str());
		}
	}
};

void TCPClient::willClosed(TCPClientConnection* connection, bool error)
{
	std::shared_ptr<ErrorCloseTask> task(new ErrorCloseTask(_questProcessor, connection, error));
	if (_questProcessor)
	{
		bool wakeup = ClientEngine::runTask(task);

		if (!wakeup)
			LOG_ERROR("wake up thread pool to process connection close event failed. Close callback will be called by Connection Reclaimer. %s", connection->_connectionInfo->str().c_str());
	}

	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (_connectionInfo.get() == connection->_connectionInfo.get())
		{
			ConnectionInfoPtr newConnectionInfo(new ConnectionInfo(0, _connectionInfo->port, _connectionInfo->ip, _isIPv4));
			_connectionInfo = newConnectionInfo;
			_connected = false;
			_connStatus = ConnStatus::NoConnected;
		}
	}

	_engine->reclaim(task);	//-- MUST after change _connectionInfo, ensure the socket hasn't been closed before _connectionInfo reset.
}

class QuestTask: public ITaskThreadPool::ITask
{
	FPQuestPtr _quest;
	ConnectionInfoPtr _connectionInfo;
	TCPClientPtr _client;
	bool _fatal;

public:
	QuestTask(TCPClientPtr client, FPQuestPtr quest, ConnectionInfoPtr connectionInfo):
		_quest(quest), _connectionInfo(connectionInfo), _client(client), _fatal(false) {}

	virtual ~QuestTask()
	{
		if (_fatal)
			_client->close();
	}

	virtual void run()
	{
		try
		{
			_client->processQuest(_quest, _connectionInfo);
		}
		catch (const FpnnError& ex){
			LOG_ERROR("processQuest() error:(%d)%s. Connection will be closed by client. %s", ex.code(), ex.what(), _connectionInfo->str().c_str());
			_fatal = true;
		}
		catch (...)
		{
			LOG_ERROR("Fatal error occurred in processQuest() function. Connection will be closed by client. %s", _connectionInfo->str().c_str());
			_fatal = true;
		}
	}
};

void TCPClient::dealQuest(FPQuestPtr quest, ConnectionInfoPtr connectionInfo)		//-- must done in thread pool or other thread.
{
	if (!_questProcessor)
	{
		LOG_ERROR("Recv a quest but without quest processor. %s", connectionInfo->str().c_str());
		return;
	}

	std::shared_ptr<QuestTask> task(new QuestTask(shared_from_this(), quest, connectionInfo));
	if (ClientEngine::runTask(task) == false)
	{
		LOG_ERROR("wake up thread pool to process quest failed. Quest pool limitation is caught. Quest task havn't be executed. %s",
			connectionInfo->str().c_str());

		if (quest->isTwoWay())
		{
			try
			{
				FPAnswerPtr answer = FPAWriter::errorAnswer(quest, FPNN_EC_CORE_WORK_QUEUE_FULL, "worker queue full", connectionInfo->str().c_str());
				std::string *raw = answer->raw();
				_engine->sendData(connectionInfo->socket, connectionInfo->token, raw);
			}
			catch (const FpnnError& ex)
			{
				LOG_ERROR("Generate error answer for duplex client worker queue full failed. No answer returned, peer need to wait timeout. %s, exception:(%d)%s",
					connectionInfo->str().c_str(), ex.code(), ex.what());
			}
			catch (...)
			{
				LOG_ERROR("Generate error answer for duplex client worker queue full failed. No answer returned, peer need to wait timeout. %s",
					connectionInfo->str().c_str());
			}
		}
	}
}

void TCPClient::dealAnswer(FPAnswerPtr answer, ConnectionInfoPtr connectionInfo)
{
	Config::ClientAnswerLog(answer, connectionInfo->ip, connectionInfo->port);

	BasicAnswerCallback* callback = ClientEngine::instance()->takeCallback(connectionInfo->socket, answer->seqNumLE());
	if (!callback)
	{
		LOG_ERROR("Recv an invalied answer, seq is %u. %s", answer->seqNumLE(), connectionInfo->str().c_str());
		return;
	}
	if (callback->syncedCallback())		//-- check first, then fill result.
	{
		SyncedAnswerCallback* sac = (SyncedAnswerCallback*)callback;
		sac->fillResult(answer, FPNN_EC_OK);
		return;
	}
	
	callback->fillResult(answer, FPNN_EC_OK);
	BasicAnswerCallbackPtr task(callback);

	if (ClientEngine::runTask(task) == false)
		LOG_ERROR("[Fatal] wake up thread pool to process answer failed. Close callback havn't called. %s", connectionInfo->str().c_str());
}

void TCPClient::processQuest(FPQuestPtr quest, ConnectionInfoPtr connectionInfo)
{
	FPAnswerPtr answer = NULL;
	_questProcessor->initAnswerStatus(connectionInfo, quest);

	try
	{
		FPReaderPtr args(new FPReader(quest->payload()));
		answer = _questProcessor->processQuest(args, quest, *connectionInfo);
	}
	catch (const FpnnError& ex){
		LOG_ERROR("processQuest ERROR:(%d) %s, connection:%s", ex.code(), ex.what(), connectionInfo->str().c_str());
		if (quest->isTwoWay())
		{
			if (_questProcessor->getQuestAnsweredStatus() == false)
				answer = FPAWriter::errorAnswer(quest, ex.code(), ex.what(), connectionInfo->str().c_str());
		}
	}
	catch (...){
		LOG_ERROR("Unknown error when calling processQuest() function. %s", connectionInfo->str().c_str());
		if (quest->isTwoWay())
		{
			if (_questProcessor->getQuestAnsweredStatus() == false)
				answer = FPAWriter::errorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, "Unknown error when calling processQuest() function", connectionInfo->str().c_str());
		}
	}

	bool questAnswered = _questProcessor->finishAnswerStatus();
	if (quest->isTwoWay())
	{
		if (questAnswered)
		{
			if (answer)
			{
				LOG_ERROR("Double answered after an advance answer sent, or async answer generated. %s", connectionInfo->str().c_str());
			}
			return;
		}
		else if (!answer)
			answer = FPAWriter::errorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, "Twoway quest lose an answer.", connectionInfo->str().c_str());
	}
	else if (answer)
	{
		LOG_ERROR("Oneway quest return an answer. %s", connectionInfo->str().c_str());
		answer = NULL;
	}

	if (answer)
	{
		std::string* raw = NULL;
		try
		{
			raw = answer->raw();
		}
		catch (const FpnnError& ex){
			FPAnswerPtr errAnswer = FPAWriter::errorAnswer(quest, ex.code(), ex.what(), connectionInfo->str().c_str());
			raw = errAnswer->raw();
		}
		catch (...)
		{
			/**  close the connection is to complex, so, return a error answer. It alway success? */

			FPAnswerPtr errAnswer = FPAWriter::errorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, "exception while do answer raw", connectionInfo->str().c_str());
			raw = errAnswer->raw();
		}

		ClientEngine::instance()->sendData(connectionInfo->socket, connectionInfo->token, raw);
	}
}

void TCPClient::close()
{
	if (!_connected)
		return;

	ConnectionInfoPtr oldConnInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		while (_connStatus == ConnStatus::Connecting || _connStatus == ConnStatus::KeyExchanging)
			_condition.wait(lck);

		if (_connStatus == ConnStatus::NoConnected)
			return;

		oldConnInfo = _connectionInfo;

		ConnectionInfoPtr newConnectionInfo(new ConnectionInfo(0, _connectionInfo->port, _connectionInfo->ip, _isIPv4));
		_connectionInfo = newConnectionInfo;
		_connected = false;
		_connStatus = ConnStatus::NoConnected;
	}

	/*
		!!! 注意 !!!
		如果在 Client::_mutex 内调用 takeConnection() 会导致在 singleClientConcurrentTset 中，
		其他线程处于发送状态时，死锁。
	*/
	TCPClientConnection* conn = _engine->takeConnection(oldConnInfo.get());
	if (conn == NULL)
		return;

	_engine->quit(conn);
	clearConnectionQuestCallbacks(conn, FPNN_EC_CORE_CONNECTION_CLOSED);
	willClosed(conn, false);
}

void TCPClient::clearConnectionQuestCallbacks(TCPClientConnection* connection, int errorCode)
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

			if (ClientEngine::runTask(task) == false)
			{
				LOG_ERROR("wake up thread pool to process quest callback when connection closing failed. Quest callback will be called in current thread. %s", connection->_connectionInfo->str().c_str());
				task->run();
			}
		}
	}
	// connection->_callbackMap.clear(); //-- If necessary.
}

FPAnswerPtr TCPClient::sendQuest(FPQuestPtr quest, int timeout)
{
	if (!_connected)
	{
		if (!_autoReconnect)
			return FPAWriter::errorAnswer(quest, FPNN_EC_CORE_CONNECTION_CLOSED, "Connection not inited.");

		if (!reconnect())
			return FPAWriter::errorAnswer(quest, FPNN_EC_CORE_CONNECTION_CLOSED, "Reconnection failed.");
	}

	ConnectionInfoPtr connInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		connInfo = _connectionInfo;
	}
	Config::ClientQuestLog(quest, connInfo->ip.c_str(), connInfo->port);

	if (timeout == 0)
		return ClientEngine::instance()->sendQuest(connInfo->socket, connInfo->token, &_mutex, quest, _timeoutQuest);
	else
		return ClientEngine::instance()->sendQuest(connInfo->socket, connInfo->token, &_mutex, quest, timeout * 1000);
}

bool TCPClient::sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout)
{
	if (!_connected)
	{
		if (!_autoReconnect)
			return false;

		if (!reconnect())
			return false;
	}

	ConnectionInfoPtr connInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		connInfo = _connectionInfo;
	}
	Config::ClientQuestLog(quest, connInfo->ip.c_str(), connInfo->port);

	if (timeout == 0)
		return ClientEngine::instance()->sendQuest(connInfo->socket, connInfo->token, quest, callback, _timeoutQuest);
	else
		return ClientEngine::instance()->sendQuest(connInfo->socket, connInfo->token, quest, callback, timeout * 1000);
}
bool TCPClient::sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout)
{
	if (!_connected)
	{
		if (!_autoReconnect)
			return false;

		if (!reconnect())
			return false;
	}

	ConnectionInfoPtr connInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		connInfo = _connectionInfo;
	}
	Config::ClientQuestLog(quest, connInfo->ip.c_str(), connInfo->port);

	if (timeout == 0)
		return ClientEngine::instance()->sendQuest(connInfo->socket, connInfo->token, quest, std::move(task), _timeoutQuest);
	else
		return ClientEngine::instance()->sendQuest(connInfo->socket, connInfo->token, quest, std::move(task), timeout * 1000);
}

bool TCPClient::configEncryptedConnection(TCPClientConnection* connection, std::string& publicKey)
{
	if (_eccCurve.empty())
		return true;

	ECCKeysMaker keysMaker;
	keysMaker.setPeerPublicKey(_serverPublicKey);
	if (keysMaker.setCurve(_eccCurve) == false)
		return false;

	publicKey = keysMaker.publicKey(true);
	if (publicKey.empty())
		return false;

	uint8_t key[32];
	uint8_t iv[16];
	if (keysMaker.calcKey(key, iv, _AESKeyLen) == false)
	{
		LOG_ERROR("Client's keys maker calcKey failed. Peer %s", connection->_connectionInfo->str().c_str());
		return false;
	}

	if (!connection->entryEncryptMode(key, _AESKeyLen, iv, !_packageEncryptionMode))
	{
		LOG_ERROR("Client connection entry encrypt mode failed. Peer %s", connection->_connectionInfo->str().c_str());
		return false;
	}
	connection->encryptAfterFirstPackageSent();
	return true;
}

ConnectionInfoPtr TCPClient::perpareConnection(int socket, std::string& publicKey)
{
	ConnectionInfoPtr newConnectionInfo;
	TCPClientConnection* connection = NULL;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		newConnectionInfo.reset(new ConnectionInfo(socket, _connectionInfo->port, _connectionInfo->ip, _isIPv4));
		connection = new TCPClientConnection(shared_from_this(), &_mutex, newConnectionInfo, _questProcessor);
	}

	if (configEncryptedConnection(connection, publicKey) == false)
	{
		delete connection;		//-- connection will close socket.
		return nullptr;
	}

	connected(connection);

	bool joined = ClientEngine::instance()->join(connection);
	if (!joined)
	{
		LOG_ERROR("Join client engine failed after connected event. %s", connection->_connectionInfo->str().c_str());
		willClosed(connection, true);

		return nullptr;
	}
	else
		return newConnectionInfo;
}

int TCPClient::connectIPv4Address(ConnectionInfoPtr currConnInfo)
{
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(currConnInfo->ip.c_str()); 
	serverAddr.sin_port = htons(currConnInfo->port);

	if (serverAddr.sin_addr.s_addr == INADDR_NONE)
		return 0;

	int socketfd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (socketfd < 0)
		return 0;

	if (::connect(socketfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) != 0)
	{
		::close(socketfd);
		return 0;
	}

	return socketfd;
}
int TCPClient::connectIPv6Address(ConnectionInfoPtr currConnInfo)
{
	struct sockaddr_in6 serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin6_family = AF_INET6;  
	serverAddr.sin6_port = htons(currConnInfo->port);

	if (inet_pton(AF_INET6, currConnInfo->ip.c_str(), &serverAddr.sin6_addr) != 1)
		return 0;

	int socketfd = ::socket(AF_INET6, SOCK_STREAM, 0);
	if (socketfd < 0)
		return 0;

	if (::connect(socketfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) != 0)
	{
		::close(socketfd);
		return 0;
	}

	return socketfd;
}

bool TCPClient::connect()
{
	if (_connected)
		return true;

	ConnectionInfoPtr oldConnInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		while (_connStatus == ConnStatus::Connecting || _connStatus == ConnStatus::KeyExchanging)
			_condition.wait(lck);

		if (_connStatus == ConnStatus::Connected)
			return true;

		oldConnInfo = _connectionInfo;

		_connected = false;
		_connStatus = ConnStatus::Connecting;
	}

	TCPClient* self = this;

	CannelableFinallyGuard errorGuard([self, oldConnInfo](){
		std::unique_lock<std::mutex> lck(self->_mutex);
		if (oldConnInfo.get() == self->_connectionInfo.get())
		{
			self->_connected = false;
			self->_connStatus = ConnStatus::NoConnected;
		}
		self->_condition.notify_all();
	});

	int socket = 0;
	if (_isIPv4)
		socket = connectIPv4Address(oldConnInfo);
	else
		socket = connectIPv6Address(oldConnInfo);

	if (socket == 0)
	{
		LOG_ERROR("Connect remote server %s failed.", oldConnInfo->str().c_str());
		return false;
	}

	std::string publicKey;
	ConnectionInfoPtr newConnInfo = perpareConnection(socket, publicKey);
	if (!newConnInfo)
		return false;

	//========= If encrypt connection ========//
	if (_eccCurve.size())
	{
		uint64_t token = newConnInfo->token;

		FPQWriter qw(3, "*key");
		qw.param("publicKey", publicKey);
		qw.param("streamMode", !_packageEncryptionMode);
		qw.param("bits", _AESKeyLen * 8); 
		FPQuestPtr quest = qw.take();

		Config::ClientQuestLog(quest, newConnInfo->ip.c_str(), newConnInfo->port);

		FPAnswerPtr answer;
		//if (timeout == 0)
			answer = ClientEngine::instance()->sendQuest(socket, token, &_mutex, quest, _timeoutQuest);
		//else
		//	answer = ClientEngine::instance()->sendQuest(socket, token, &_mutex, quest, timeout * 1000);

		if (answer->status() != 0)
		{
			LOG_ERROR("Client's key exchanging failed. Peer %s", newConnInfo->str().c_str());
			
			TCPClientConnection* conn = _engine->takeConnection(newConnInfo.get());
			if (conn == NULL)
				return false;

			_engine->quit(conn);
			clearConnectionQuestCallbacks(conn, FPNN_EC_CORE_UNKNOWN_ERROR);
			willClosed(conn, true);
			return false;
		}
	}

	errorGuard.cancel();
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (_connectionInfo.get() == oldConnInfo.get())
		{
			_connectionInfo = newConnInfo;
			_connected = true;
			_connStatus = ConnStatus::Connected;
			_condition.notify_all();

			return true;
		}
	}

	//-- dupled
	TCPClientConnection* conn = _engine->takeConnection(newConnInfo.get());
	if (conn)
	{
		_engine->quit(conn);
		clearConnectionQuestCallbacks(conn, FPNN_EC_CORE_CONNECTION_CLOSED);
		willClosed(conn, false);
	}

	std::unique_lock<std::mutex> lck(_mutex);

	while (_connStatus == ConnStatus::Connecting || _connStatus == ConnStatus::KeyExchanging)
		_condition.wait(lck);

	_condition.notify_all();
	if (_connStatus == ConnStatus::Connected)
		return true;

	return false;
}
bool TCPClient::reconnect()
{
	close();
	return connect();
}
