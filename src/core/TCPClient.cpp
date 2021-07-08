#include <exception>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include "Config.h"
#include "ClientEngine.h"
#include "FPWriter.h"
#include "AutoRelease.h"
#include "FileSystemUtil.h"
#include "PEM_DER_SAX.h"
#include "TCPClient.h"

using namespace fpnn;

TCPClient::TCPClient(const std::string& host, int port, bool autoReconnect):
	Client(host, port, autoReconnect), _AESKeyLen(16), _packageEncryptionMode(true),
	_keepAliveParams(NULL), _connectTimeout(0)
{
	if (Config::Client::KeepAlive::defaultEnable)
		keepAlive();
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

void TCPClient::keepAlive()
{
	std::unique_lock<std::mutex> lck(_mutex);

	if (!_keepAliveParams)
	{
		_keepAliveParams = new TCPClientKeepAliveParams;

		_keepAliveParams->pingTimeout = 0;
		_keepAliveParams->pingInterval = Config::Client::KeepAlive::pingInterval;
		_keepAliveParams->maxPingRetryCount = Config::Client::KeepAlive::maxPingRetryCount;
	}
}
void TCPClient::setKeepAlivePingTimeout(int seconds)
{
	keepAlive();
	_keepAliveParams->pingTimeout = seconds * 1000;
}
void TCPClient::setKeepAliveInterval(int seconds)
{
	keepAlive();
	_keepAliveParams->pingInterval = seconds * 1000;
}
void TCPClient::setKeepAliveMaxPingRetryCount(int count)
{
	keepAlive();
	_keepAliveParams->maxPingRetryCount = count;
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
		catch (const std::exception& ex)
		{
			LOG_ERROR("processQuest() error: %s. Connection will be closed by client. %s", ex.what(), _connectionInfo->str().c_str());
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
		LOG_ERROR("wake up thread pool to process TCP quest failed. Quest pool limitation is caught. Quest task havn't be executed. %s",
			connectionInfo->str().c_str());

		if (quest->isTwoWay())
		{
			try
			{
				FPAnswerPtr answer = FPAWriter::errorAnswer(quest, FPNN_EC_CORE_WORK_QUEUE_FULL, std::string("worker queue full, ") + connectionInfo->str().c_str());
				std::string *raw = answer->raw();
				_engine->sendTCPData(connectionInfo->socket, connectionInfo->token, raw);
			}
			catch (const FpnnError& ex)
			{
				LOG_ERROR("Generate error answer for TCP duplex client worker queue full failed. No answer returned, peer need to wait timeout. %s, exception:(%d)%s",
					connectionInfo->str().c_str(), ex.code(), ex.what());
			}
			catch (const std::exception& ex)
			{
				LOG_ERROR("Generate error answer for TCP duplex client worker queue full failed. No answer returned, peer need to wait timeout. %s, exception: %s",
					connectionInfo->str().c_str(), ex.what());
			}
			catch (...)
			{
				LOG_ERROR("Generate error answer for TCP duplex client worker queue full failed. No answer returned, peer need to wait timeout. %s",
					connectionInfo->str().c_str());
			}
		}
	}
}

void TCPClient::close()
{
	if (!_connected)
		return;

	std::list<AsyncQuestCacheUnit*> asyncQuestCache;
	std::list<std::string*> asyncEmbedDataCache;

	ConnectionInfoPtr oldConnInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		//while (_connStatus == ConnStatus::Connecting)
		//	_condition.wait(lck);

		if (_connStatus == ConnStatus::NoConnected)
			return;

		bool notify = _connStatus == ConnStatus::Connecting;

		oldConnInfo = _connectionInfo;

		ConnectionInfoPtr newConnectionInfo(new ConnectionInfo(0, _connectionInfo->port, _connectionInfo->ip, _isIPv4));
		_connectionInfo = newConnectionInfo;
		_connected = false;
		_connStatus = ConnStatus::NoConnected;

		if (_requireCacheSendData)
		{
			asyncQuestCache.swap(_asyncQuestCache);
			asyncEmbedDataCache.swap(_asyncEmbedDataCache);
			_requireCacheSendData = false;
		}

		if (notify)
			_condition.notify_all();
	}

	failedCachedSendingData(oldConnInfo, asyncQuestCache, asyncEmbedDataCache);

	/*
		!!! 注意 !!!
		如果在 Client::_mutex 内调用 takeConnection() 会导致在 singleClientConcurrentTset 中，
		其他线程处于发送状态时，死锁。
	*/
	TCPClientConnection* conn = (TCPClientConnection*)_engine->takeConnection(oldConnInfo.get());
	if (conn == NULL)
		return;

	_engine->quit(conn);
	clearConnectionQuestCallbacks(conn, FPNN_EC_CORE_CONNECTION_CLOSED);
	willClose(conn, false);
}

FPAnswerPtr TCPClient::sendQuest(FPQuestPtr quest, int timeout)
{
	if (!_connected)
	{
		if (!_autoReconnect)
		{
			if (quest->isTwoWay())
				return FpnnErrorAnswer(quest, FPNN_EC_CORE_CONNECTION_CLOSED, "Client is not allowed auto-connected.");
			else
				return NULL;
		}

		if (quest->isOneWay())
		{
			sendQuest(quest, NULL, timeout);
			return NULL;
		}

		if (!reconnect())
		{
			if (quest->isTwoWay())
				return FpnnErrorAnswer(quest, FPNN_EC_CORE_CONNECTION_CLOSED, "Reconnection failed.");
			else
				return NULL;
		}
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

		if (!asyncReconnect())
			return false;
	}

	ConnectionInfoPtr connInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (_requireCacheSendData)
		{
			cacheSendQuest(quest, callback, timeout);
			return true;
		}
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

		if (!asyncReconnect())
			return false;
	}

	ConnectionInfoPtr connInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (_requireCacheSendData)
		{
			BasicAnswerCallback* callback = new FunctionAnswerCallback(std::move(task));
			cacheSendQuest(quest, callback, timeout);
			return true;
		}

		connInfo = _connectionInfo;
	}
	Config::ClientQuestLog(quest, connInfo->ip.c_str(), connInfo->port);

	if (timeout == 0)
		return ClientEngine::instance()->sendQuest(connInfo->socket, connInfo->token, quest, std::move(task), _timeoutQuest);
	else
		return ClientEngine::instance()->sendQuest(connInfo->socket, connInfo->token, quest, std::move(task), timeout * 1000);
}

	/*===============================================================================
	  Interfaces for embed mode.
	=============================================================================== */
bool TCPClient::embed_sendData(std::string* rawData)
{
	if (!_connected)
	{
		if (!_autoReconnect)
			return false;

		if (!asyncReconnect())
			return false;
	}

	ConnectionInfoPtr connInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (_requireCacheSendData)
		{
			_asyncEmbedDataCache.push_back(rawData);
			return true;
		}

		connInfo = _connectionInfo;
	}
	//-- Config::ClientQuestLog(quest, connInfo->ip.c_str(), connInfo->port);
	ClientEngine::instance()->sendTCPData(connInfo->socket, connInfo->token, rawData);
	return true;
}

bool TCPClient::configEncryptedConnection(TCPClientConnection* connection, std::string& publicKey)
{
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

class TCPConnectionECDHCallback: public AnswerCallback
{
	TCPClientConnection* _connection;
public:
	TCPConnectionECDHCallback(TCPClientConnection* conn): _connection(conn) { conn->_refCount++; }
	~TCPConnectionECDHCallback() { _connection->_refCount--; }

	virtual void onAnswer(FPAnswerPtr) {}
	virtual void onException(FPAnswerPtr answer, int errorCode)
	{
		TCPClientPtr client = _connection->client();
		if (client)
		{
			LOG_ERROR("TCP client's key exchanging failed. Peer %s", _connection->_connectionInfo->str().c_str());

			TCPClientConnection* conn = (TCPClientConnection*)ClientEngine::instance()->takeConnection(_connection->socket());
			if (conn == NULL)
				return;

			ClientEngine::instance()->quit(conn);
			client->clearConnectionQuestCallbacks(conn, FPNN_EC_CORE_INVALID_CONNECTION);
			client->willClose(conn, true);
		}
		else
		{
			//-- 此时已经在回收队列中。回收在 Client 调用 close() 或者 析构的时候被触发。
		}
	}
};

void TCPClient::sendEncryptHandshake(TCPClientConnection* connection, const std::string& publicKey)
{
	ConnectionInfoPtr connInfo = connection->_connectionInfo;

	FPQWriter qw(3, "*key");
	qw.param("publicKey", publicKey);
	qw.param("streamMode", !_packageEncryptionMode);
	qw.param("bits", _AESKeyLen * 8); 
	FPQuestPtr quest = qw.take();

	Config::ClientQuestLog(quest, connInfo->ip.c_str(), connInfo->port);

	TCPConnectionECDHCallback* callback = new TCPConnectionECDHCallback(connection);

	std::string* raw = quest->raw();
	uint32_t seqNum = quest->seqNumLE();

	int timeout = _timeoutQuest;
	if (timeout == 0)
		timeout = ClientEngine::getQuestTimeout() * 1000;

	callback->updateExpiredTime(TimeUtil::curr_msec() + timeout);

	connection->_callbackMap[seqNum] = callback;
	connection->_sendBuffer.appendData(raw);
}

void TCPClient::cacheSendQuest(FPQuestPtr quest, BasicAnswerCallback* callback, int timeout)
{
	AsyncQuestCacheUnit* unit = new AsyncQuestCacheUnit();
	unit->quest = quest;
	unit->timeoutMS = timeout * 1000;
	unit->callback = callback;
	_asyncQuestCache.push_back(unit);
}

void TCPClient::dumpCachedSendData(ConnectionInfoPtr connInfo)
{
	std::list<AsyncQuestCacheUnit*> asyncQuestCache;
	std::list<std::string*> asyncEmbedDataCache;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		asyncQuestCache.swap(_asyncQuestCache);
		asyncEmbedDataCache.swap(_asyncEmbedDataCache);
		_requireCacheSendData = false;
	}

	std::list<BasicAnswerCallback*> failedCallbacks;

	for (auto unit: asyncQuestCache)
	{
		if (unit->timeoutMS == 0)
			unit->timeoutMS = _timeoutQuest;

		 Config::ClientQuestLog(unit->quest, connInfo->ip.c_str(), connInfo->port);

		if (_engine->sendQuest(connInfo->socket, connInfo->token, unit->quest, unit->callback, unit->timeoutMS) == false)
			if (unit->callback)
				failedCallbacks.push_back(unit->callback);

		delete unit;
	}

	for (auto data: asyncEmbedDataCache)
	{
		_engine->sendTCPData(connInfo->socket, connInfo->token, data);
	}

	for (auto callback: failedCallbacks)
	{
		/*
		*	当前场景，不会存在同步 callback。
		*
		if (callback->syncedCallback())		//-- check first, then fill result.
		{
			SyncedAnswerCallback* sac = (SyncedAnswerCallback*)callback;
			sac->fillResult(NULL, FPNN_EC_CORE_INVALID_CONNECTION);
			continue;
		}*/
		
		callback->fillResult(NULL, FPNN_EC_CORE_INVALID_CONNECTION);
		BasicAnswerCallbackPtr task(callback);

		if (ClientEngine::runTask(task) == false)
			LOG_ERROR("[Fatal] wake up thread pool to process cached quest in async mode failed. Callback havn't called. %s", connInfo->str().c_str());
	}
}

bool TCPClient::perpareConnection(int socket, bool connected, ConnectionInfoPtr currConnInfo)
{
	ConnectionInfoPtr newConnInfo;
	TCPClientConnection* connection = NULL;
	{
		std::unique_lock<std::mutex> lck(_mutex);

		newConnInfo.reset(new ConnectionInfo(socket, currConnInfo->port, currConnInfo->ip, _isIPv4));
		connection = new TCPClientConnection(shared_from_this(), newConnInfo, _questProcessor);

		if (_connectTimeout > 0)
			connection->_connectingExpiredMS = _connectTimeout;
		else
			connection->_connectingExpiredMS = ClientEngine::getConnectTimeout() * 1000;

		connection->_connectingExpiredMS += slack_real_msec();

		if (_keepAliveParams)
		{
			if (_keepAliveParams->pingTimeout)
				connection->configKeepAlive(_keepAliveParams);
			else
			{
				_keepAliveParams->pingTimeout = _timeoutQuest ? _timeoutQuest : ClientEngine::getQuestTimeout() * 1000;
				connection->configKeepAlive(_keepAliveParams);
				_keepAliveParams->pingTimeout = 0;
			}
		}

		if (_embedRecvNotifyDeleagte)
			connection->embed_configRecvNotifyDelegate(_embedRecvNotifyDeleagte);

		_connectionInfo = newConnInfo;
	}

	if (_eccCurve.size())
	{
		std::string publicKey;
		if (configEncryptedConnection(connection, publicKey) == false)
		{
			// delete connection;		//-- connection will close socket.

			LOG_ERROR("TCP client config encryption for remote server %s failed.", newConnInfo->str().c_str());
			connectFailed(newConnInfo, FPNN_EC_CORE_INVALID_CONNECTION);
			reclaim(connection, true);
			return false;
		}

		sendEncryptHandshake(connection, publicKey);
	}
	
	connection->disableIOOperations();

	if (connected)
		connection->_socketConnected = true;

	if (ClientEngine::instance()->join(connection, !connected) == false)
	{
		if (_eccCurve.size())
			delete connection->_callbackMap.begin()->second;	//-- ECDH callback

		// delete connection;		//-- connection will close socket.

		LOG_ERROR("TCP client join engine failed. %s", newConnInfo->str().c_str());
		connectFailed(newConnInfo, FPNN_EC_CORE_INVALID_CONNECTION);
		reclaim(connection, true);
		return false;
	}

	dumpCachedSendData(newConnInfo);

	if (connected)
		socketConnected(connection, true);

	return true;
}

void TCPClient::socketConnected(TCPClientConnection* conn, bool connected)
{
	conn->_refCount++;
	TCPClientPtr self = shared_from_this();

	bool status = ClientEngine::runTask([conn, self, connected](){

		FinallyGuard guard([conn]() {
			conn->_refCount--;
		});

		if (connected)
		{
			bool requireCallConnectionCannelledEvent;
			if (conn->getConnectedEventCallingPermission(requireCallConnectionCannelledEvent) == false)
				return;

			int errorCode = FPNN_EC_OK;
			if (requireCallConnectionCannelledEvent == false)
			{
				if (self->connectSuccessed(conn))
				{
					self->callConnectedEvent(conn, true);

					bool connectionDiscarded = false;
					conn->connectedEventCalled(connectionDiscarded);
					if (!connectionDiscarded)
					{
						conn->connectedEventCompleted();
					}
					else
					{
						IQuestProcessorPtr processor = conn->questProcessor();
						if (processor)
						{
							try
							{
								processor->connectionWillClose(*(conn->_connectionInfo), false);
							}
							catch (const FpnnError& ex){
								LOG_ERROR("TCPClient call connection close event error:(%d)%s. %s", ex.code(), ex.what(), conn->_connectionInfo->str().c_str());
							}
							catch (...)
							{
								LOG_ERROR("Unknown error when calling connection close event. %s", conn->_connectionInfo->str().c_str());
							}
						}
					}

					return;
				}
				else
				{
					conn->connectionDiscarded();
					errorCode = FPNN_EC_CORE_CONNECTION_CLOSED;
					//-- 转到：链接被取消清理工作
				}
			}
			else
			{
				errorCode = FPNN_EC_CORE_CANCELLED;
				//-- 转到：链接被取消清理工作
			}

			//-- 链接被取消清理工作
			self->connectFailed(conn->_connectionInfo, errorCode);
			self->callConnectedEvent(conn, false);

			TCPClientConnection* connection = (TCPClientConnection*)ClientEngine::instance()->takeConnection(conn->_connectionInfo.get());
			if (connection != NULL)
			{
				ClientEngine::instance()->quit(connection);
				ClientEngine::instance()->clearConnectionQuestCallbacks(connection, errorCode);
				self->willClose(connection, false);
			}
		}
		else	//-- 连接建立失败。
		{
			int errorCode = FPNN_EC_CORE_INVALID_CONNECTION;
			self->connectFailed(conn->_connectionInfo, errorCode);

			bool requireCallConnectionCannelledEvent;
			if (conn->getConnectedEventCallingPermission(requireCallConnectionCannelledEvent))
			{
				self->callConnectedEvent(conn, false);

				TCPClientConnection* connection = (TCPClientConnection*)ClientEngine::instance()->takeConnection(conn->_connectionInfo.get());
				if (connection != NULL)
				{
					ClientEngine::instance()->quit(connection);
					ClientEngine::instance()->clearConnectionQuestCallbacks(connection, errorCode);
					self->willClose(connection, false);
				}
			}
			//-- else case is connecting be cannelled. 
		}
	});

	if (!status)
		conn->_refCount--;
}

int TCPClient::connectIPv4Address(ConnectionInfoPtr currConnInfo, bool& connected)
{
	connected = false;
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

	if (!nonblockedFd(socketfd))
	{
		::close(socketfd);
		LOG_ERROR("TCP client change socket to nonblocking for remote server %s failed.", currConnInfo->str().c_str());
		return 0;
	}

	if (::connect(socketfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) != 0)
	{
		if (errno == EINPROGRESS)
			return socketfd;
		
		::close(socketfd);
		LOG_ERROR("TCP client async connect to remote server %s failed. error: %d", currConnInfo->str().c_str(), errno);
		return 0;
	}
	else
	{
		connected = true;
		return socketfd;
	}
}
int TCPClient::connectIPv6Address(ConnectionInfoPtr currConnInfo, bool& connected)
{
	connected = false;
	struct sockaddr_in6 serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin6_family = AF_INET6;  
	serverAddr.sin6_port = htons(currConnInfo->port);

	if (inet_pton(AF_INET6, currConnInfo->ip.c_str(), &serverAddr.sin6_addr) != 1)
		return 0;

	int socketfd = ::socket(AF_INET6, SOCK_STREAM, 0);
	if (socketfd < 0)
		return 0;

	if (!nonblockedFd(socketfd))
	{
		::close(socketfd);
		LOG_ERROR("TCP client change socket to nonblocking for remote server %s failed.", currConnInfo->str().c_str());
		return 0;
	}

	if (::connect(socketfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) != 0)
	{
		if (errno == EINPROGRESS)
			return socketfd;
		
		::close(socketfd);
		LOG_ERROR("TCP client async connect to remote server %s failed. error: %d", currConnInfo->str().c_str(), errno);
		return 0;
	}
	else
	{
		connected = true;
		return socketfd;
	}
}

bool TCPClient::connect()
{
	if (_connected)
		return true;

	if (!asyncConnect())
		return false;

	{
		std::unique_lock<std::mutex> lck(_mutex);
		while (_connStatus == ConnStatus::Connecting)
			_condition.wait(lck);

		if (_connStatus == ConnStatus::Connected)
			return true;

		return false;
	}
}

void TCPClient::connectFailed(ConnectionInfoPtr connInfo, int errorCode)
{
	std::list<AsyncQuestCacheUnit*> asyncQuestCache;
	std::list<std::string*> asyncEmbedDataCache;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (connInfo.get() != _connectionInfo.get())
			return;

		ConnectionInfoPtr newConnectionInfo(new ConnectionInfo(0, _connectionInfo->port, _connectionInfo->ip, _isIPv4));
		_connectionInfo = newConnectionInfo;
		_connected = false;
		_connStatus = ConnStatus::NoConnected;

		asyncQuestCache.swap(_asyncQuestCache);
		asyncEmbedDataCache.swap(_asyncEmbedDataCache);
		_requireCacheSendData = false;

		_condition.notify_all();
	}

	failedCachedSendingData(connInfo, asyncQuestCache, asyncEmbedDataCache);
}

bool TCPClient::connectSuccessed(TCPClientConnection* conn)
{
	std::unique_lock<std::mutex> lck(_mutex);
	if (_connectionInfo.get() == conn->_connectionInfo.get())
	{
		_connectionInfo = conn->_connectionInfo;
		_connected = true;
		_connStatus = ConnStatus::Connected;
		_condition.notify_all();
		return true;
	}
	return false;
}

bool TCPClient::asyncConnect()
{
	if (_connected)
		return true;

	//-- prepare
	ConnectionInfoPtr currentConnInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);

		if (_connStatus == ConnStatus::Connected || _connStatus == ConnStatus::Connecting)
			return true;

		currentConnInfo = _connectionInfo;

		_connected = false;
		_requireCacheSendData = true;
		_connStatus = ConnStatus::Connecting;
	}

	//-- begin connect
	bool connected = false;
	int socket = 0;
	if (_isIPv4)
		socket = connectIPv4Address(currentConnInfo, connected);
	else
		socket = connectIPv6Address(currentConnInfo, connected);

	if (socket == 0)
	{
		LOG_ERROR("TCP client connect remote server %s failed.", currentConnInfo->str().c_str());
		connectFailed(currentConnInfo, FPNN_EC_CORE_INVALID_CONNECTION);
		triggerConnectingFailedEvent(currentConnInfo, FPNN_EC_CORE_INVALID_CONNECTION);
		return false;
	}

	return perpareConnection(socket, connected, currentConnInfo);
}

void TCPClient::triggerConnectingFailedEvent(ConnectionInfoPtr connInfo, int errorCode)
{
	if (_questProcessor)
	{
		IQuestProcessorPtr processor = _questProcessor;
		bool status = ClientEngine::runTask([processor, connInfo, errorCode](){

			(void)errorCode;

			try
			{
				processor->connected(*connInfo, false);
			}
			catch (const FpnnError& ex){
				LOG_ERROR("Call connecting failed event error:(%d)%s. %s", ex.code(), ex.what(), connInfo->str().c_str());
			}
			catch (const std::exception& ex)
			{
				LOG_ERROR("Call connecting failed event error: %s. %s", ex.what(), connInfo->str().c_str());
			}
			catch (...)
			{
				LOG_ERROR("Unknown error when calling connecting failed event function. %s", connInfo->str().c_str());
			}
		});

		if (!status)
			LOG_ERROR("Launch connecting failed event failed. %s", connInfo->str().c_str());		
	}
}

TCPClientPtr Client::createTCPClient(const std::string& host, int port, bool autoReconnect)
{
	return TCPClient::createClient(host, port, autoReconnect);
}
TCPClientPtr Client::createTCPClient(const std::string& endpoint, bool autoReconnect)
{
	return TCPClient::createClient(endpoint, autoReconnect);
}
