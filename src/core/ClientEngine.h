#ifndef FPNN_Client_Engine_H
#define FPNN_Client_Engine_H

#include <mutex>
#include <atomic>
#include <memory>
#include <thread>
#include <set>
#include "FPLog.h"
#include "IOWorker.h"
#include "TaskThreadPool.h"
#include "TCPClientIOWorker.h"
#include "IQuestProcessor.h"
#include "ConnectionMap.h"
#include "ConcurrentSenderInterface.h"

namespace fpnn
{
	class ClientEngine;
	typedef std::shared_ptr<ClientEngine> ClientEnginePtr;

	struct ClientEngineInitParams
	{
		int globalConnectTimeoutSeconds;
		int globalQuestTimeoutSeconds;
		int residentTaskThread;
		int maxTaskThreads;

		bool ignoreSignals;

		ClientEngineInitParams(): globalConnectTimeoutSeconds(5), globalQuestTimeoutSeconds(5), residentTaskThread(4), maxTaskThreads(64), ignoreSignals(true) {}
	};

	class ClientEngine: virtual public IConcurrentSender
	{
	private:
		std::mutex _mutex;
		FPLogPtr _logHolder;

		int _notifyFds[2];
		int _connectTimeout;
		int _questTimeout;
		std::atomic<bool> _running;
		std::set<int> _newSocketSet;
		std::set<int> _waitWriteSet;
		std::set<int> _quitSocketSet;

		bool _newSocketSetChanged;
		bool _waitWriteSetChanged;
		bool _quitSocketSetChanged;

		ConnectionMap _connectionMap;
		TaskThreadPool _callbackPool;

		std::set<IReleaseablePtr> _reclaimedConnections;

		std::thread _timeoutChecker;
		std::thread _loopThread;

		std::atomic<uint64_t> _loopTicket;

		ClientEngine(const ClientEngineInitParams *params = NULL);

		void closeUDPConnection(UDPClientConnection* connection);
		void clearConnection(int socket, int errorCode);
		void reclaimConnections();
		void clearTimeoutQuest();
		void clean();
		void loopThread();
		void consumeNotifyData();
		void timeoutCheckThread();
		void processConnectionIO(int fd, bool canRead, bool canWrite);

	public:
		static ClientEnginePtr create(const ClientEngineInitParams* params = NULL);
		static ClientEnginePtr instance() { return create(); }
		virtual ~ClientEngine();

		inline static void setQuestTimeout(int seconds)
		{
			instance()->_questTimeout = seconds * 1000;
		}
		inline static int getQuestTimeout(){
			return instance()->_questTimeout / 1000;
		}
		inline static void setConnectTimeout(int seconds)
		{
			instance()->_connectTimeout = seconds * 1000;
		}
		inline static int getConnectTimeout(){
			return instance()->_connectTimeout / 1000;
		}

		inline static bool runTask(std::shared_ptr<ITaskThreadPool::ITask> task)
		{
			return instance()->_callbackPool.wakeUp(task);
		}

		inline static bool runTask(std::function<void ()> task)
		{
			return instance()->_callbackPool.wakeUp(std::move(task));
		}

		void clearConnectionQuestCallbacks(BasicConnection*, int errorCode);

		inline BasicConnection* takeConnection(const ConnectionInfo* ci)  //-- !!! Using for other case. e.g. TCPCLient.
		{
			return _connectionMap.takeConnection(ci);
		}
		inline BasicConnection* takeConnection(int socket)
		{
			return _connectionMap.takeConnection(socket);
		}
		inline BasicAnswerCallback* takeCallback(int socket, uint32_t seqNum)
		{
			return _connectionMap.takeCallback(socket, seqNum);
		}
		inline bool embed_checkCallback(int socket, uint32_t seqNum)
		{
			return _connectionMap.embed_checkCallback(socket, seqNum);
		}

		bool join(const BasicConnection* connection, bool waitForSending);
		bool waitSendEvent(const BasicConnection* connection);
		void quit(BasicConnection* connection);

		inline void keepAlive(int socket, bool keepAlive)		//-- Only for ARQ UDP
		{
			_connectionMap.keepAlive(socket, keepAlive);
		}
		inline void setUDPUntransmittedSeconds(int socket, int untransmittedSeconds)		//-- Only for ARQ UDP
		{
			_connectionMap.setUDPUntransmittedSeconds(socket, untransmittedSeconds);
		}
		inline void executeConnectionAction(int socket, std::function<void (BasicConnection* conn)> action)		//-- Only for ARQ UDP
		{
			_connectionMap.executeConnectionAction(socket, std::move(action));
		}

		virtual void sendTCPData(int socket, uint64_t token, std::string* data);
		virtual void sendUDPData(int socket, uint64_t token, std::string* data, int64_t expiredMS, bool discardable);
		
		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.
		*/
		virtual FPAnswerPtr sendQuest(int socket, uint64_t token, std::mutex* mutex, FPQuestPtr quest, int timeout = 0)
		{
			if (timeout == 0) timeout = _questTimeout;
			return _connectionMap.sendQuest(socket, token, mutex, quest, timeout, quest->isOneWay());
		}
		virtual bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, AnswerCallback* callback, int timeout = 0)
		{
			if (timeout == 0) timeout = _questTimeout;
			return _connectionMap.sendQuest(socket, token, quest, callback, timeout, quest->isOneWay());
		}
		virtual bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, BasicAnswerCallback* callback, int timeout = 0)
		{
			if (timeout == 0) timeout = _questTimeout;
			return _connectionMap.sendQuest(socket, token, quest, callback, timeout, quest->isOneWay());
		}
		virtual bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		{
			if (timeout == 0) timeout = _questTimeout;
			return _connectionMap.sendQuest(socket, token, quest, std::move(task), timeout, quest->isOneWay());
		}

		//-- For UDP Client
		virtual FPAnswerPtr sendQuest(int socket, uint64_t token, std::mutex* mutex, FPQuestPtr quest, int timeout, bool discardableUDPQuest)
		{
			if (timeout == 0) timeout = _questTimeout;
			return _connectionMap.sendQuest(socket, token, mutex, quest, timeout, discardableUDPQuest);
		}
		virtual bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, AnswerCallback* callback, int timeout, bool discardableUDPQuest)
		{
			if (timeout == 0) timeout = _questTimeout;
			return _connectionMap.sendQuest(socket, token, quest, callback, timeout, discardableUDPQuest);
		}
		virtual bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout, bool discardableUDPQuest)
		{
			if (timeout == 0) timeout = _questTimeout;
			return _connectionMap.sendQuest(socket, token, quest, std::move(task), timeout, discardableUDPQuest);
		}

		inline void reclaim(IReleaseablePtr object)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			_reclaimedConnections.insert(object);
		}
	};

	class ClientCloseTask: virtual public ITaskThreadPool::ITask, virtual public IReleaseable
	{
		bool _error;
		bool _executed;
		BasicConnection* _connection;
		IQuestProcessorPtr _questProcessor;

	public:
		ClientCloseTask(IQuestProcessorPtr questProcessor, BasicConnection* connection, bool error):
			_error(error), _executed(false), _connection(connection), _questProcessor(questProcessor)
			{
				connection->connectionDiscarded();
			}

		virtual ~ClientCloseTask()
		{
			if (!_executed)
				run();

			delete _connection;
		}

		virtual bool releaseable(uint64_t currentLoopTicket) { return _connection->releaseable(currentLoopTicket); }
		virtual void run();
	};
}

#endif
