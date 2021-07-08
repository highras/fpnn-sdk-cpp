#ifndef FPNN_TCP_Client_H
#define FPNN_TCP_Client_H

#include "StringUtil.h"
#include "NetworkUtility.h"
#include "TCPClientIOWorker.h"
#include "KeyExchange.h"
#include "ClientInterface.h"

namespace fpnn
{
	class TCPClient;
	typedef std::shared_ptr<TCPClient> TCPClientPtr;

	//=================================================================//
	//- TCP Client:
	//=================================================================//
	class TCPClient: public Client, public std::enable_shared_from_this<TCPClient>
	{
	private:
		int _AESKeyLen;
		bool _packageEncryptionMode;
		std::string _eccCurve;
		std::string _serverPublicKey;
		//----------
		TCPClientKeepAliveParams* _keepAliveParams;
		//----------
		int _connectTimeout;
		std::list<AsyncQuestCacheUnit*> _questCache;

	private:
		bool configEncryptedConnection(TCPClientConnection* connection, std::string& publicKey);
		void sendEncryptHandshake(TCPClientConnection* connection, const std::string& publicKey);
		int connectIPv4Address(ConnectionInfoPtr currConnInfo, bool& connected);
		int connectIPv6Address(ConnectionInfoPtr currConnInfo, bool& connected);
		bool perpareConnection(int socket, bool connected, ConnectionInfoPtr currConnInfo);
		void cacheSendQuest(FPQuestPtr quest, BasicAnswerCallback* callback, int timeout);
		void dumpCachedSendData(ConnectionInfoPtr connInfo);
		void triggerConnectingFailedEvent(ConnectionInfoPtr connInfo, int errorCode);

		TCPClient(const std::string& host, int port, bool autoReconnect = true);

	public:
		virtual ~TCPClient()
		{
			if (_keepAliveParams)
				delete _keepAliveParams;
		}

		/*===============================================================================
		  Call by framwwork.
		=============================================================================== */
		void dealQuest(FPQuestPtr quest, ConnectionInfoPtr connectionInfo);
		void socketConnected(TCPClientConnection* conn, bool connected);
		void connectFailed(ConnectionInfoPtr connInfo, int errorCode);
		bool connectSuccessed(TCPClientConnection* conn);

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

		bool enableEncryptorByDerData(const std::string &derData, bool packageMode = true, bool reinforce = false);
		bool enableEncryptorByPemData(const std::string &PemData, bool packageMode = true, bool reinforce = false);
		bool enableEncryptorByDerFile(const char *derFilePath, bool packageMode = true, bool reinforce = false);
		bool enableEncryptorByPemFile(const char *pemFilePath, bool packageMode = true, bool reinforce = false);

		virtual void keepAlive();
		void setKeepAlivePingTimeout(int seconds);
		void setKeepAliveInterval(int seconds);
		void setKeepAliveMaxPingRetryCount(int count);

		inline void setConnectTimeout(int seconds)
		{
			_connectTimeout = seconds * 1000;
		}
		inline int getConnectTimeout()
		{
			return _connectTimeout / 1000;
		}
		/*===============================================================================
		  Call by Developer.
		=============================================================================== */
		virtual bool connect();
		virtual bool asyncConnect();
		virtual void close();

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.

			timeout in seconds.
		*/
		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

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

		/*===============================================================================
		  Interfaces for embed mode.
		=============================================================================== */
		/*
		* 如果返回成功，rawData 由 C++ SDK 负责释放(delete), 如果失败，需要调用者释放。
		*/
		virtual bool embed_sendData(std::string* rawData);
	};
}

#endif
