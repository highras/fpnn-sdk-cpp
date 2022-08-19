#ifndef FPNN_UDP_Client_H
#define FPNN_UDP_Client_H

#include "StringUtil.h"
#include "NetworkUtility.h"
#include "UDPClientIOWorker.h"
#include "ClientInterface.h"

namespace fpnn
{
	class UDPClient;
	typedef std::shared_ptr<UDPClient> UDPClientPtr;

	//=================================================================//
	//- UDP Client:
	//=================================================================//
	class UDPClient: public Client, public std::enable_shared_from_this<UDPClient>
	{
		int _MTU;
		bool _keepAlive;
		int _untransmittedSeconds;

		//----------
		std::string _eccCurve;
		std::string _serverPublicKey;
		bool _packageReinforce;
		bool _dataEnhance;
		bool _dataReinforce;

	private:
		int connectIPv4Address(ConnectionInfoPtr currConnInfo);
		int connectIPv6Address(ConnectionInfoPtr currConnInfo);
		bool perpareConnection(ConnectionInfoPtr currConnInfo);

		UDPClient(const std::string& host, int port, bool autoReconnect = true);

	public:
		virtual ~UDPClient() {}

		/*===============================================================================
		  Call by framwwork.
		=============================================================================== */
		void dealQuest(FPQuestPtr quest, ConnectionInfoPtr connectionInfo);		//-- must done in thread pool or other thread.
		/*===============================================================================
		  Call by Developer. Configure Function.
		=============================================================================== */
		bool enableEncryptorByDerData(const std::string &derData, bool packageReinforce = false,
			bool dataEnhance = false, bool dataReinforce = false);
		bool enableEncryptorByPemData(const std::string &PemData, bool packageReinforce = false,
			bool dataEnhance = false, bool dataReinforce = false);
		bool enableEncryptorByDerFile(const char *derFilePath, bool packageReinforce = false,
			bool dataEnhance = false, bool dataReinforce = false);
		bool enableEncryptorByPemFile(const char *pemFilePath, bool packageReinforce = false,
			bool dataEnhance = false, bool dataReinforce = false);
		inline void enableEncryptor(const std::string& curve, const std::string& peerPublicKey,
			bool packageReinforce = false, bool dataEnhance = false, bool dataReinforce = false)
		{
			_eccCurve = curve;
			_serverPublicKey = peerPublicKey;
			_packageReinforce = packageReinforce;
			_dataEnhance = dataEnhance;
			_dataReinforce = dataReinforce;
		}
		/*===============================================================================
		  Call by Developer.
		=============================================================================== */
		/*
		*	因为 UDP Client 的异步连接操作，只有**连接建立事件**需要异步。但一般情况下，几乎不会用到该事件。
		*	因此为了简便起见，asyncConnect() 直接调用 connect()。
		*	除非有特殊需求，再做成 TCP Client 那样复杂的纯异步操作。
		*/
		virtual bool asyncConnect() { return connect(); }
		virtual bool connect();
		virtual void close();
		
		inline static UDPClientPtr createClient(const std::string& host, int port, bool autoReconnect = true)
		{
			return UDPClientPtr(new UDPClient(host, port, autoReconnect));
		}
		inline static UDPClientPtr createClient(const std::string& endpoint, bool autoReconnect = true)
		{
			std::string host;
			int port;

			if (!parseAddress(endpoint, host, port))
				return nullptr;

			return UDPClientPtr(new UDPClient(host, port, autoReconnect));
		}

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.

			timeout in seconds.
			Oneway will deal as discardable, towway will deal as reliable.
		*/
		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0)
		{
			return sendQuestEx(quest, quest->isOneWay(), timeout * 1000);
		}
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0)
		{
			return sendQuestEx(quest, callback, quest->isOneWay(), timeout * 1000);
		}
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		{
			return sendQuestEx(quest, std::move(task), quest->isOneWay(), timeout * 1000);
		}

		//-- Timeout in milliseconds
		virtual FPAnswerPtr sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec = 0);

		virtual void keepAlive();
		void setUntransmittedSeconds(int untransmittedSeconds);		//-- 0: using default; -1: disable.
		void setMTU(int MTU) { _MTU = MTU; }		//-- Please called before connect() or sendQuest().

		/*===============================================================================
		  Interfaces for embed mode.
		=============================================================================== */
		/*
		* 如果返回成功，rawData 由 C++ SDK 负责释放(delete), 如果失败，需要调用者释放。
		*/
		virtual bool embed_sendData(std::string* rawData, bool discardable, int timeoutMsec = 0);
		virtual bool embed_sendData(std::string* rawData) { return embed_sendData(rawData, false, 0); }
	};
}

#endif