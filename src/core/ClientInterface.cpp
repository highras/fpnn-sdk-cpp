#include "Config.h"
#include "NetworkUtility.h"
#include "ClientInterface.h"

using namespace fpnn;

const char* Client::SDKVersion = FPNN_SDK_VERSION;

Client::Client(const std::string& host, int port, bool autoReconnect): _connected(false),
	_connStatus(ConnStatus::NoConnected), _timeoutQuest(0), _autoReconnect(autoReconnect),
	_requireCacheSendData(false), _embedRecvNotifyDeleagte(NULL)
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

Client::~Client()
{
	if (_connected)
		close();
}

void Client::close()
{
	LOG_FATAL("Please implement the close() function for client inherited from fpnn::Client class.");
}

void Client::failedCachedSendingData(ConnectionInfoPtr connectionInfo,
	std::list<AsyncQuestCacheUnit*>& asyncQuestCache, std::list<std::string*>& asyncEmbedDataCache)
{
	for (auto unit: asyncQuestCache)
	{
		if (unit->callback)
		{
			if (unit->callback->syncedCallback())		//-- check first, then fill result.
			{
				SyncedAnswerCallback* sac = (SyncedAnswerCallback*)(unit->callback);
				sac->fillResult(NULL, FPNN_EC_CORE_INVALID_CONNECTION);
				continue;
			}
			
			unit->callback->fillResult(NULL, FPNN_EC_CORE_INVALID_CONNECTION);
			BasicAnswerCallbackPtr task(unit->callback);

			if (ClientEngine::runTask(task) == false)
				LOG_ERROR("[Fatal] wake up thread pool to process cached quest in async mode failed. Callback havn't called. %s", connectionInfo->str().c_str());
		}

		delete unit;
	}

	for (auto data: asyncEmbedDataCache)
	{
		delete data;
		LOG_ERROR("Embed data not send at socket %d. Connecting maybe prepare error or be cannelled.", connectionInfo->socket);
	}
}

void Client::callConnectedEvent(BasicConnection* connection, bool connected)
{
	if (_questProcessor)
	{
		try
		{
			_questProcessor->connected(*(connection->_connectionInfo), connected);
		}
		catch (const FpnnError& ex){
			LOG_ERROR("connected() error:(%d)%s. %s", ex.code(), ex.what(), connection->_connectionInfo->str().c_str());
		}
		catch (const std::exception& ex)
		{
			LOG_ERROR("connected() error: %s. %s", ex.what(), connection->_connectionInfo->str().c_str());
		}
		catch (...)
		{
			LOG_ERROR("Unknown error when calling connected() function. %s", connection->_connectionInfo->str().c_str());
		}
	}
}

void Client::reclaim(BasicConnection* connection, bool error)
{
	std::shared_ptr<ClientCloseTask> task(new ClientCloseTask(_questProcessor, connection, error));
	if (_questProcessor)
	{
		bool wakeup = ClientEngine::runTask(task);
		if (!wakeup)
			LOG_ERROR("wake up thread pool to process connection close event failed. Close callback will be called by Connection Reclaimer. %s", connection->_connectionInfo->str().c_str());
	}

	_engine->reclaim(task);
}

void Client::willClose(BasicConnection* connection, bool error)
{	
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

	//-- MUST after change _connectionInfo, ensure the socket hasn't been closed before _connectionInfo reset.
	reclaim(connection, error);
}

void Client::dealAnswer(FPAnswerPtr answer, ConnectionInfoPtr connectionInfo)
{
	Config::ClientAnswerLog(answer, connectionInfo->ip, connectionInfo->port);

	BasicAnswerCallback* callback = ClientEngine::instance()->takeCallback(connectionInfo->socket, answer->seqNumLE());
	if (!callback)
	{
		LOG_WARN("Recv an invalid answer, seq: %u. Peer %s, Info: %s", answer->seqNumLE(), connectionInfo->str().c_str(), answer->info().c_str());
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

void Client::processQuest(FPQuestPtr quest, ConnectionInfoPtr connectionInfo)
{
	FPAnswerPtr answer = NULL;
	_questProcessor->initAnswerStatus(connectionInfo, quest);

	try
	{
		FPReaderPtr args(new FPReader(quest->payload()));
		answer = _questProcessor->processQuest(args, quest, *connectionInfo);
	}
	catch (const FpnnError& ex)
	{
		LOG_ERROR("processQuest ERROR:(%d) %s, connection:%s", ex.code(), ex.what(), connectionInfo->str().c_str());
		if (quest->isTwoWay())
		{
			if (_questProcessor->getQuestAnsweredStatus() == false)
				answer = FpnnErrorAnswer(quest, ex.code(), std::string(ex.what()) + ", " + connectionInfo->str());
		}
	}
	catch (const std::exception& ex)
	{
		LOG_ERROR("processQuest ERROR: %s, connection:%s", ex.what(), connectionInfo->str().c_str());
		if (quest->isTwoWay())
		{
			if (_questProcessor->getQuestAnsweredStatus() == false)
				answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string(ex.what()) + ", " + connectionInfo->str());
		}
	}
	catch (...){
		LOG_ERROR("Unknown error when calling processQuest() function. %s", connectionInfo->str().c_str());
		if (quest->isTwoWay())
		{
			if (_questProcessor->getQuestAnsweredStatus() == false)
				answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string("Unknown error when calling processQuest() function, ") + connectionInfo->str());
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
			answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string("Twoway quest lose an answer. ") + connectionInfo->str());
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
			FPAnswerPtr errAnswer = FpnnErrorAnswer(quest, ex.code(), std::string(ex.what()) + ", " + connectionInfo->str());
			raw = errAnswer->raw();
		}
		catch (const std::exception& ex)
		{
			FPAnswerPtr errAnswer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string(ex.what()) + ", " + connectionInfo->str());
			raw = errAnswer->raw();
		}
		catch (...)
		{
			/**  close the connection is to complex, so, return a error answer. It alway success? */

			FPAnswerPtr errAnswer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string("exception while do answer raw, ") + connectionInfo->str());
			raw = errAnswer->raw();
		}

		if (connectionInfo->isTCP())
			ClientEngine::instance()->sendTCPData(connectionInfo->socket, connectionInfo->token, raw);
		else
			ClientEngine::instance()->sendUDPData(connectionInfo->socket, connectionInfo->token, raw, 0, quest->isOneWay());
	}
}

void Client::clearConnectionQuestCallbacks(BasicConnection* connection, int errorCode)
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

bool Client::reconnect()
{
	close();
	return connect();
}

bool Client::asyncReconnect()
{
	close();
	return asyncConnect();
}
