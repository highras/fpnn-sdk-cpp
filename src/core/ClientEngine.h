#ifndef FPNN_Client_Engine_H
#define FPNN_Client_Engine_H

#include <mutex>
#include <atomic>
#include <memory>
#include <thread>
#include <set>
#include "FPLog.h"
#include "TaskThreadPool.h"
#include "ClientIOWorker.h"
#include "IQuestProcessor.h"
#include "ConnectionMap.h"
#include "ConcurrentSenderInterface.h"

namespace fpnn
{
	class ClientEngine;
	typedef std::shared_ptr<ClientEngine> ClientEnginePtr;

	struct ClientEngineInitParams
	{
		int globalTimeoutSeconds;
		int residentTaskThread;
		int maxTaskThreads;

		bool ignoreSignals;

		ClientEngineInitParams(): globalTimeoutSeconds(5), residentTaskThread(4), maxTaskThreads(64), ignoreSignals(true) {}
	};

	class ClientEngine: virtual public IConcurrentSender
	{
	private:
		static std::mutex _mutex;
		FPLogPtr _logHolder;

		int _notifyFds[2];
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

		ClientEngine(const ClientEngineInitParams *params = NULL);

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
		~ClientEngine();

		inline static void setQuestTimeout(int seconds)
		{
			instance()->_questTimeout = seconds * 1000;
		}
		inline static int getQuestTimeout(){
			return instance()->_questTimeout / 1000;
		}

		inline static bool runTask(std::shared_ptr<ITaskThreadPool::ITask> task)
		{
			return instance()->_callbackPool.wakeUp(task);
		}

		void clearConnectionQuestCallbacks(TCPClientConnection*, int errorCode);

		inline TCPClientConnection* takeConnection(const ConnectionInfo* ci)  //-- !!! Using for other case. e.g. TCPCLient.
		{
			return (TCPClientConnection*)_connectionMap.takeConnection(ci);
		}
		inline BasicAnswerCallback* takeCallback(int socket, uint32_t seqNum)
		{
			return _connectionMap.takeCallback(socket, seqNum);
		}

		bool join(const TCPClientConnection* connection);
		bool waitSendEvent(const TCPClientConnection* connection);
		void quit(const TCPClientConnection* connection);

		virtual void sendData(int socket, uint64_t token, std::string* data);
		
		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.
		*/
		virtual FPAnswerPtr sendQuest(int socket, uint64_t token, std::mutex* mutex, FPQuestPtr quest, int timeout = 0)
		{
			if (timeout == 0) timeout = _questTimeout;
			return _connectionMap.sendQuest(socket, token, mutex, quest, timeout);
		}
		virtual bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, AnswerCallback* callback, int timeout = 0)
		{
			if (timeout == 0) timeout = _questTimeout;
			return _connectionMap.sendQuest(socket, token, quest, callback, timeout);
		}
		virtual bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		{
			if (timeout == 0) timeout = _questTimeout;
			return _connectionMap.sendQuest(socket, token, quest, std::move(task), timeout);
		}

		inline void reclaim(IReleaseablePtr object)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			_reclaimedConnections.insert(object);
		}
	};

	class CloseErrorTask: virtual public ITaskThreadPool::ITask, virtual public IReleaseable
	{
		bool _error;
		TCPClientConnection* _connection;

	public:
		CloseErrorTask(TCPClientConnection* connection, bool error): _error(error), _connection(connection) {}

		virtual ~CloseErrorTask() { delete _connection; }
		virtual bool releaseable() { return (_connection->_refCount == 0); }
		virtual void run();
	};
	typedef std::shared_ptr<CloseErrorTask> CloseErrorTaskPtr;
}

#endif
