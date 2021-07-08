#ifndef FPNN_Client_Interface_H
#define FPNN_Client_Interface_H

#include <atomic>
#include <mutex>
#include <memory>
#include <functional>
#include <condition_variable>
#include "AnswerCallbacks.h"
#include "ClientEngine.h"
#include "IQuestProcessor.h"
#include "embedTypes.h"

namespace fpnn
{
	class Client;
	typedef std::shared_ptr<Client> ClientPtr;

	struct AsyncQuestCacheUnit
	{
		FPQuestPtr quest;
		int64_t timeoutMS;
		BasicAnswerCallback* callback;
		bool discardable;

		AsyncQuestCacheUnit(): timeoutMS(0), callback(NULL), discardable(false) {}
	};

	class Client
	{
	public:
		static const char* SDKVersion;

	protected:
		enum class ConnStatus
		{
			NoConnected,
			Connecting,
			Connected,
		};

	protected:
		std::mutex _mutex;
		std::condition_variable _condition;		//-- Only for connected.
		bool _isIPv4;
		std::atomic<bool> _connected;
		ConnStatus _connStatus;
		ClientEnginePtr _engine;

		IQuestProcessorPtr _questProcessor;
		ConnectionInfoPtr	_connectionInfo;
		std::string _endpoint;

		int64_t _timeoutQuest;
		bool _autoReconnect;

		bool _requireCacheSendData;
		std::list<AsyncQuestCacheUnit*> _asyncQuestCache;
		std::list<std::string*> _asyncEmbedDataCache;

		EmbedRecvNotifyDelegate _embedRecvNotifyDeleagte;

	protected:
		void reclaim(BasicConnection* connection, bool error);

	public:
		Client(const std::string& host, int port, bool autoReconnect = true);
		virtual ~Client();
		/*===============================================================================
		  Call by framwwork.
		=============================================================================== */
		void callConnectedEvent(BasicConnection* connection, bool connected);
		void willClose(BasicConnection* connection, bool error);		//-- must done in thread pool or other thread.
		void dealAnswer(FPAnswerPtr answer, ConnectionInfoPtr connectionInfo);		//-- must done in thread pool or other thread.
		void processQuest(FPQuestPtr quest, ConnectionInfoPtr connectionInfo);
		void clearConnectionQuestCallbacks(BasicConnection* connection, int errorCode);
		void failedCachedSendingData(ConnectionInfoPtr connectionInfo,
			std::list<AsyncQuestCacheUnit*>& asyncQuestCache, std::list<std::string*>& asyncEmbedDataCache);
		/*===============================================================================
		  Call by anybody.
		=============================================================================== */
		inline bool connected() { return _connected; }
		inline const std::string& endpoint() { return _endpoint; }
		inline int socket()
		{
			std::unique_lock<std::mutex> lck(_mutex);
			return _connectionInfo->socket;
		}
		inline ConnectionInfoPtr connectionInfo()
		{
			std::unique_lock<std::mutex> lck(_mutex);
			return _connectionInfo;
		}
		/*===============================================================================
		  Call by Developer. Configure Function.
		=============================================================================== */
		inline void setQuestProcessor(IQuestProcessorPtr processor)
		{
			_questProcessor = processor;
			_questProcessor->setConcurrentSender(_engine.get());
		}

		inline void setQuestTimeout(int64_t seconds)
		{
			_timeoutQuest = seconds * 1000;
		}
		inline int64_t getQuestTimeout()
		{
			return _timeoutQuest / 1000;
		}

		inline bool isAutoReconnect()
		{
			return _autoReconnect;
		}
		inline void setAutoReconnect(bool autoReconnect)
		{
			_autoReconnect = autoReconnect;
		}

		/*===============================================================================
		  Call by Developer.
		=============================================================================== */
		virtual bool connect() = 0;
		virtual bool asyncConnect() = 0;
		virtual void close();				//-- Please MUST implement this 'close()' function for specific implementations.
		virtual bool reconnect();
		virtual bool asyncReconnect();
		virtual void keepAlive() = 0;

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.

			timeout in seconds.
		*/
		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0) = 0;
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0) = 0;
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0) = 0;

		static TCPClientPtr createTCPClient(const std::string& host, int port, bool autoReconnect = true);
		static TCPClientPtr createTCPClient(const std::string& endpoint, bool autoReconnect = true);

		static UDPClientPtr createUDPClient(const std::string& host, int port, bool autoReconnect = true);
		static UDPClientPtr createUDPClient(const std::string& endpoint, bool autoReconnect = true);

		/*===============================================================================
		  Interfaces for embed mode.
		=============================================================================== */
		/*
		* 链接建立和关闭事件，用 IQuestProcessor 封装。
		* server push / duplex，采用 EmbedRecvNotifyDelegate 通知接口，由上层语言/封装处理。
		*/
		virtual void embed_configRecvNotifyDelegate(EmbedRecvNotifyDelegate delegate) { _embedRecvNotifyDeleagte = delegate; }

		/*
		* 如果返回成功，rawData 由 C++ SDK 负责释放(delete), 如果失败，需要调用者释放。
		*/
		virtual bool embed_sendData(std::string* rawData) = 0;
	};
}

#endif