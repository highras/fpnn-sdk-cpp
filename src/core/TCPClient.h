#ifndef FPNN_TCP_Client_H
#define FPNN_TCP_Client_H

#include <atomic>
#include <mutex>
#include <memory>
#include <functional>
#include <condition_variable>
#include "AnswerCallbacks.h"
#include "StringUtil.h"
#include "NetworkUtility.h"
#include "ClientEngine.h"
#include "ClientIOWorker.h"
#include "IQuestProcessor.h"
#include "KeyExchange.h"

namespace fpnn
{
	class TCPClient;
	typedef std::shared_ptr<TCPClient> TCPClientPtr;

	//=================================================================//
	//- TCP Client:
	//=================================================================//
	class TCPClient: public std::enable_shared_from_this<TCPClient>
	{
		enum class ConnStatus
		{
			NoConnected,
			Connecting,
			KeyExchanging,
			Connected,
		};
	private:
		std::mutex _mutex;
		std::condition_variable _condition;		//-- Only for connected.
		std::atomic<bool> _isIPv4;
		std::atomic<bool> _connected;			//-- quickly than _connStatus. But _connStatus is necessary, _connected is optional.
		ConnStatus _connStatus;
		ClientEnginePtr _engine;

		IQuestProcessorPtr _questProcessor;
		ConnectionInfoPtr	_connectionInfo;
		std::string _endpoint;

		int _timeoutQuest;

		//----------
		int _AESKeyLen;
		bool _packageEncryptionMode;
		std::string _eccCurve;
		std::string _serverPublicKey;

		//----------
		std::atomic<bool> _autoReconnect;

	private:
		int connectIPv4Address(ConnectionInfoPtr currConnInfo);
		int connectIPv6Address(ConnectionInfoPtr currConnInfo);
		ConnectionInfoPtr perpareConnection(int socket, std::string& publicKey);
		
		bool configEncryptedConnection(TCPClientConnection* connection, std::string& publicKey);

		TCPClient(const std::string& host, int port, bool autoReconnect = true);

	public:
		~TCPClient();

		/*===============================================================================
		  Call by framwwork.
		=============================================================================== */
		void connected(TCPClientConnection* connection);
		void willClosed(TCPClientConnection* connection, bool error);	//-- must done in thread pool or other thread.
		void dealQuest(FPQuestPtr quest, ConnectionInfoPtr connectionInfo);
		void dealAnswer(FPAnswerPtr answer, ConnectionInfoPtr connectionInfo);		//-- must done in thread pool or other thread.
		void processQuest(FPQuestPtr quest, ConnectionInfoPtr connectionInfo);
		void clearConnectionQuestCallbacks(TCPClientConnection* connection, int errorCode);
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
		inline void enableEncryptor(const std::string& curve, const std::string& peerPublicKey, bool packageMode = true, bool reinforce = false)
		{
			_eccCurve = curve;
			_serverPublicKey = peerPublicKey;
			_packageEncryptionMode = packageMode;
			_AESKeyLen = reinforce ? 32 : 16;
		}
		inline void setQuestProcessor(IQuestProcessorPtr processor)
		{
			_questProcessor = processor;
			_questProcessor->setConcurrentSender(_engine.get());
		}

		inline void setQuestTimeout(int seconds)
		{
			_timeoutQuest = seconds * 1000;
		}
		inline int getQuestTimeout()
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
		bool enableEncryptorByDerData(const std::string &derData, bool packageMode = true, bool reinforce = false);
		bool enableEncryptorByPemData(const std::string &PemData, bool packageMode = true, bool reinforce = false);
		bool enableEncryptorByDerFile(const char *derFilePath, bool packageMode = true, bool reinforce = false);
		bool enableEncryptorByPemFile(const char *pemFilePath, bool packageMode = true, bool reinforce = false);
		/*===============================================================================
		  Call by Developer.
		=============================================================================== */
		void close();

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.

			timeout in seconds.
		*/
		FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
		bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

		inline static TCPClientPtr createClient(const std::string& host, int port, bool autoReconnect = true)
		{
			return TCPClientPtr(new TCPClient(host, port, autoReconnect));
		}
		inline static TCPClientPtr createClient(const std::string& endpoint, bool autoReconnect = true)
		{
			std::string host;
			int port;

			if (!parseAddress(endpoint, host, port))
				return nullptr;

			return TCPClientPtr(new TCPClient(host, port, autoReconnect));
		}

		bool connect();
		bool reconnect();
	};
}

#endif
