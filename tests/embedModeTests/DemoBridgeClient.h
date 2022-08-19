#ifndef Bridge_Client_h
#define Bridge_Client_h

#include <iostream>
#include "fpnn.h"
#include "Decoder.h"

using namespace fpnn;

//========================================================================================//
//- Real header File Content
//========================================================================================//
class DemoBridgeClient;

void DemoEmbedRecvNotifyDelegate(uint64_t connectionId, const void* buffer, int length);

//=================================================================//
//- Demo Answer Callback
//=================================================================//
class DemoAnswerCallback
{
private:
	std::function<void (FPAnswerPtr answer, int errorCode)> _function;
	int64_t _expireTime;

public:
	explicit DemoAnswerCallback(std::function<void (FPAnswerPtr answer, int errorCode)> function):
		_function(function) {}
	virtual ~DemoAnswerCallback() {}

	virtual void call(FPAnswerPtr answer, int errorCode) final
	{
		_function(answer, errorCode);
	}

	bool isExpired() { return _expireTime <= TimeUtil::curr_msec(); }
	void setExpiredTime(int timeoutSecond)
	{
		_expireTime = TimeUtil::curr_msec() + timeoutSecond * 1000;
	}
};

//=================================================================//
//- Demo Server Push Callback
//=================================================================//
class DemoServerPushCallback
{
private:
	std::function<FPAnswerPtr (uint64_t connectionId, const std::string& endpoint,
		bool isTCP, bool encrypted, FPQuestPtr quest)> _function;

public:
	explicit DemoServerPushCallback(std::function<FPAnswerPtr (uint64_t connectionId, const std::string& endpoint,
		bool isTCP, bool encrypted, FPQuestPtr quest)> function): _function(function) {}
	virtual ~DemoServerPushCallback() {}

	virtual FPAnswerPtr call(uint64_t connectionId, const std::string& endpoint,
		bool isTCP, bool encrypted, FPQuestPtr quest) final
	{
		return _function(connectionId, endpoint, isTCP, encrypted, quest);
	}
};

//=================================================================//
//- Demo Thread Pool
//=================================================================//
class DemoThreadPool
{
	class AnswerCallbackRunner: public ITaskThreadPool::ITask
	{
		DemoAnswerCallback* _cb;
		FPAnswerPtr _answer;
		int _errorCode;

	public:
		AnswerCallbackRunner(DemoAnswerCallback* cb, FPAnswerPtr answer, int errorCode):
			_cb(cb), _answer(answer), _errorCode(errorCode) {}
		virtual ~AnswerCallbackRunner() {}
		virtual void run()
		{
			_cb->call(_answer, _errorCode);
			delete _cb;
		}
	};

public:
	static void runAnswerCallback(DemoAnswerCallback* cb, FPAnswerPtr answer, int errorCode)
	{
		std::shared_ptr<AnswerCallbackRunner> task = std::make_shared<AnswerCallbackRunner>(cb, answer, errorCode);
		ClientEngine::runTask(task);
	}

	static void runTask(std::function<void ()> task)
	{
		std::shared_ptr<ITaskThreadPool::ITask> t(new TaskThreadPool::FunctionTask(std::move(task)));
		ClientEngine::runTask(t);
	}
};

//=================================================================//
//- Demo Client Manager：全剧管理器。可以没有，但有了更方便
//=================================================================//
class DemoClientManager
{
	std::mutex _mutex;
	std::thread _loop;
	std::atomic<bool> _running;
	std::unordered_set<DemoBridgeClient*> _clients;
	std::unordered_map<uint64_t, DemoBridgeClient*> _connClientMap;

	void loopThread();

	DemoBridgeClient* findConnectionClient(uint64_t connectionId)
	{
		auto it = _connClientMap.find(connectionId);
		if (it != _connClientMap.end())
			return it->second;
		else
		{
			std::cout<<" --[ERROR]--: cannot found client for connection ID: "<<connectionId<<" for received data."<<std::endl;
			return NULL;
		}
	}

public:
	void registerClient(DemoBridgeClient* client)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		_clients.insert(client);
	}
	void unregisterClient(DemoBridgeClient* client)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		_clients.erase(client);
	}

	void registerConnection(uint64_t connectionId, DemoBridgeClient* client)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		_connClientMap[connectionId] = client;
	}

	void unregisterConnection(uint64_t connectionId)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		_connClientMap.erase(connectionId);
	}

	void dealReceivedAnswer(uint64_t connectionId, FPAnswerPtr answer);
	void dealReceivedQuest(uint64_t connectionId, FPQuestPtr quest);

	void start()
	{
		_running = true;
		_loop = std::thread(&DemoClientManager::loopThread, this);
	}

	DemoClientManager(): _running(false) {}
	~DemoClientManager()
	{
		if (!_running)
			return;

		_running = false;
		_loop.join();
	}
};

//=================================================================//
//- Demo Quest Processor: 链接事件封装演示
//=================================================================//
class DemoQuestProcessor: public IQuestProcessor
{
	QuestProcessorClassPrivateFields(DemoQuestProcessor)

	DemoBridgeClient* _client;

	std::function<void (uint64_t connectionId, const std::string& endpoint,
		bool isTCP, bool encrypted, bool connected)> _connectedEvent;

	std::function<void (uint64_t connectionId, const std::string& endpoint,
		bool isTCP, bool encrypted, bool closedByError)> _closedEvent;

public:
	DemoQuestProcessor(DemoBridgeClient* client): IQuestProcessor(), _client(client) {}
	virtual ~DemoQuestProcessor() {}

	virtual void connected(const ConnectionInfo& ci, bool connected);
	virtual void connectionWillClose(const ConnectionInfo& ci, bool closeByError);

	void setConnectedNotify(std::function<void (uint64_t connectionId, const std::string& endpoint,
		bool isTCP, bool encrypted, bool connected)> func)
	{
		_connectedEvent = std::move(func);
	}

	void setConnectionClosedNotify(std::function<void (uint64_t connectionId, const std::string& endpoint,
		bool isTCP, bool encrypted, bool closedByError)> func)
	{
		_closedEvent = std::move(func);
	}

	QuestProcessorClassBasicPublicFuncs
};

//=================================================================//
//- Demo Bridge Client：客户端封装演示
//=================================================================//
class DemoBridgeClient
{
	bool _isTCP;
	std::mutex _mutex;
	ClientPtr _sdkClient;
	std::shared_ptr<DemoQuestProcessor> _questProcessor;
	std::unordered_map<uint32_t, DemoAnswerCallback*> _callbackMap;
	std::unordered_map<std::string, DemoServerPushCallback*> _duplexCallbackMap;

public:
	DemoBridgeClient(const std::string& ip, int port, bool tcp);
	~DemoBridgeClient();

	uint64_t getCurrentConnectionId()
	{
		return _sdkClient->connectionInfo()->uniqueId();
	}

	inline ConnectionInfoPtr connectionInfo()
	{
		return _sdkClient->connectionInfo();
	}

	inline void setQuestTimeout(int64_t seconds)
	{
		_sdkClient->setQuestTimeout(seconds);
	}

	bool enableEncryption(const char *pemFilePath)
	{
		if (_isTCP)
			return ((TCPClient*)(_sdkClient.get()))->enableEncryptorByPemFile(pemFilePath);
		else
			return ((UDPClient*)(_sdkClient.get()))->enableEncryptorByPemFile(pemFilePath);
	}

	bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> callback, int timeout = 0)
	{
		int realTimeout = timeout;
		if (realTimeout == 0)
			realTimeout = _sdkClient->getQuestTimeout();
		if (realTimeout == 0)
			realTimeout = ClientEngine::instance()->getQuestTimeout();

		DemoAnswerCallback* cb = new DemoAnswerCallback(std::move(callback));
		cb->setExpiredTime(realTimeout);

		uint32_t seqNum = quest->seqNumLE();

		{
			std::unique_lock<std::mutex> lck(_mutex);
			_callbackMap[seqNum] = cb;
		}

		std::string* raw = quest->raw();
		if (_sdkClient->embed_sendData(raw))
			return true;
		else
		{
			delete raw;
			std::unique_lock<std::mutex> lck(_mutex);
			_callbackMap.erase(seqNum);
			return false;
		}
	}

	bool sendOnewayQuest(FPQuestPtr quest)
	{
		std::string* raw = quest->raw();
		if (_sdkClient->embed_sendData(raw))
			return true;
		else
		{
			delete raw;
			return false;
		}
	}

	bool sendAnswer(FPAnswerPtr answer)
	{
		std::string* raw = answer->raw();
		if (_sdkClient->embed_sendData(raw))
			return true;
		else
		{
			delete raw;
			return false;
		}
	}

	void keepAlive()
	{
		_sdkClient->keepAlive();
	}

	bool connect()
	{
		return _sdkClient->connect();
	}

	void close()
	{
		_sdkClient->close();
		std::unordered_map<uint32_t, DemoAnswerCallback*> newMap;

		{
			std::unique_lock<std::mutex> lck(_mutex);
			newMap.swap(_callbackMap);
		}
			
		for (auto& pp: newMap)
			DemoThreadPool::runAnswerCallback(pp.second, nullptr, FPNN_EC_CORE_CONNECTION_CLOSED);
	}

	//-- 链接事件
	void setConnectedNotify(std::function<void (uint64_t connectionId, const std::string& endpoint,
		bool isTCP, bool encrypted, bool connected)> func)
	{
		_questProcessor->setConnectedNotify(std::move(func));
	}

	void setConnectionClosedNotify(std::function<void (uint64_t connectionId, const std::string& endpoint,
		bool isTCP, bool encrypted, bool closedByError)> func)
	{
		_questProcessor->setConnectionClosedNotify(std::move(func));
	}

	//-- server Push
	void registerServerPush(const std::string& method, std::function<FPAnswerPtr (uint64_t connectionId,
		const std::string& endpoint, bool isTCP, bool encrypted, FPQuestPtr quest)> func)
	{
		if (_duplexCallbackMap.find(method) != _duplexCallbackMap.end())
			delete _duplexCallbackMap[method];

		_duplexCallbackMap[method] = new DemoServerPushCallback(std::move(func));
	}

public:
	//-- 管理部分
	DemoAnswerCallback* takeAnswerCallback(FPAnswerPtr answer)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		uint32_t seqNum = answer->seqNumLE();
		auto it = _callbackMap.find(seqNum);
		if (it != _callbackMap.end())
		{
			DemoAnswerCallback* cb = it->second;
			_callbackMap.erase(it);
			return cb;
		}

		return NULL;
	}

	void takeExpiredAnswerCallback(std::list<DemoAnswerCallback*>& expiredCallbacks)
	{
		std::list<uint32_t> expiredSeqNums;

		std::unique_lock<std::mutex> lck(_mutex);
		for (auto& pp: _callbackMap)
		{
			if (pp.second->isExpired())
			{
				expiredSeqNums.push_back(pp.first);
				expiredCallbacks.push_back(pp.second);
			}
		}

		for (auto seqNum: expiredSeqNums)
			_callbackMap.erase(seqNum);
	}

	DemoServerPushCallback* findServerPushBack(const std::string& method)
	{
		auto it = _duplexCallbackMap.find(method);
		if (it != _duplexCallbackMap.end())
			return it->second;
		else
			return NULL;
	}
};

//========================================================================================//
//- Real CPP File Content
//========================================================================================//ß

void DemoClientManager::loopThread()
{
	while (_running)
	{
		sleep(1);

		std::list<DemoAnswerCallback*> expiredCallbacks;
		{
			std::unique_lock<std::mutex> lck(_mutex);
			for (auto client: _clients)
			{
				client->takeExpiredAnswerCallback(expiredCallbacks);
			}
		}

		for (auto cb: expiredCallbacks)
			DemoThreadPool::runAnswerCallback(cb, nullptr, FPNN_EC_CORE_TIMEOUT);
	}
}

void DemoClientManager::dealReceivedAnswer(uint64_t connectionId, FPAnswerPtr answer)
{
	DemoAnswerCallback* cb = NULL;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		DemoBridgeClient* client = findConnectionClient(connectionId);
		if (client == NULL)
			return;

		cb = client->takeAnswerCallback(answer);
		if (cb == NULL)
		{
			std::cout<<" --[Info]--: received invalid answer."<<std::endl;
			return;
		}
	}

	int errCode = FPNN_EC_OK;
	if (answer->status())
	{
		FPAReader ar(answer);
		errCode = ar.wantInt("code");
	}

	DemoThreadPool::runAnswerCallback(cb, answer, errCode);
}

void DemoClientManager::dealReceivedQuest(uint64_t connectionId, FPQuestPtr quest)
{
	ConnectionInfoPtr ci;
	DemoBridgeClient* client = NULL;
	DemoServerPushCallback* cb = NULL;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		client = findConnectionClient(connectionId);
		if (client == NULL)
			return;

		cb = client->findServerPushBack(quest->method());
		if (cb == NULL)
		{
			std::cout<<" --[ERROR]--: method '"<<quest->method()<<"'' cannot be found."<<std::endl;
			return;
		}

		ci = client->connectionInfo();
	}

	DemoThreadPool::runTask([client, cb, connectionId, ci, quest](){
		FPAnswerPtr answer = cb->call(connectionId, ci->endpoint(), ci->isTCP(),
			ci->isEncrypted(), quest);

		/*
		*	！！！警告！！！
		*
		*	（1）这里为了演示方便，没有控制 client 的有效性。
		*	实际中，此时 client 可能已经被销毁， client 指针已经失效。
		*
		*	（2）这里为了演示方便，没有检查请求是 oneway 还是 twoway，直接判断了 answer 的有效性，然后直接发送。
		*	实际中，应该先检查 quest 是 oneway 还是 twoway，再决定 answer 的判断和处理。
		*/

		if (answer)
			client->sendAnswer(answer);
	});
}

DemoClientManager demoGlobalClientManager;

void DemoQuestProcessor::connected(const ConnectionInfo& ci, bool connected)
{
	demoGlobalClientManager.registerConnection(ci.uniqueId(), _client);

	if (_connectedEvent)
		_connectedEvent(ci.uniqueId(), ci.endpoint(), ci.isTCP(), ci.isEncrypted(), connected);
}
void DemoQuestProcessor::connectionWillClose(const ConnectionInfo& ci, bool closeByError)
{
	if (_closedEvent)
		_closedEvent(ci.uniqueId(), ci.endpoint(), ci.isTCP(), ci.isEncrypted(), closeByError);

	demoGlobalClientManager.unregisterConnection(ci.uniqueId());
}

DemoBridgeClient::DemoBridgeClient(const std::string& ip, int port, bool tcp): _isTCP(tcp)
{
	if (tcp)
		_sdkClient = Client::createTCPClient(ip, port);
	else
		_sdkClient = Client::createUDPClient(ip, port);

	_questProcessor = std::make_shared<DemoQuestProcessor>(this);
	_sdkClient->setQuestProcessor(_questProcessor);

	_sdkClient->embed_configRecvNotifyDelegate(DemoEmbedRecvNotifyDelegate);

	demoGlobalClientManager.registerClient(this);
}
DemoBridgeClient::~DemoBridgeClient()
{
	demoGlobalClientManager.unregisterClient(this);

	close();

	for (auto& pp: _duplexCallbackMap)
		delete pp.second;
}

void DemoEmbedRecvNotifyDelegate(uint64_t connectionId, const void* buffer, int length)
{
	try
	{
		if (FPMessage::isQuest((char *)buffer))
		{
			FPQuestPtr quest = Decoder::decodeQuest((char *)buffer, length);
			demoGlobalClientManager.dealReceivedQuest(connectionId, quest);
		}
		else
		{
			FPAnswerPtr answer = Decoder::decodeAnswer((char *)buffer, length);
			demoGlobalClientManager.dealReceivedAnswer(connectionId, answer);
		}
	}
	catch (const FpnnError& ex)
	{
		LOG_ERROR("Decode error. Code: %d, error: %s.", ex.code(), ex.what());
	}
	catch (...)
	{
		LOG_ERROR("Decode error.");
	}
}

#endif