#ifndef UDP_Connection_h
#define UDP_Connection_h

#include "IOWorker.h"
#include "UDPIOBuffer.h"

namespace fpnn
{
	class UDPClient;
	typedef std::shared_ptr<UDPClient> UDPClientPtr;

	//===============[ UDPClientConnection ]=====================//
	class UDPClientConnection: public BasicConnection
	{
		UDPIOBuffer _ioBuffer;
		std::weak_ptr<UDPClient> _client;

	public:
		UDPClientConnection(UDPClientPtr client, ConnectionInfoPtr connectionInfo, IQuestProcessorPtr questProcessor, int MTU):
			BasicConnection(connectionInfo), _ioBuffer(NULL, connectionInfo->socket, MTU), _client(client)
		{
			_questProcessor = questProcessor;
			_connectionInfo->token = (uint64_t)this;	//-- if use Virtual Derive, must redo this in subclass constructor.
			_connectionInfo->_mutex = &_mutex;
			_ioBuffer.initMutex(&_mutex);
			_ioBuffer.updateEndpointInfo(_connectionInfo->endpoint());
		}

		virtual ~UDPClientConnection() {}

		virtual bool waitForSendEvent();
		virtual enum ConnectionType connectionType() { return BasicConnection::UDPClientConnectionType; }
		UDPClientPtr client() { return _client.lock(); }

		inline void enableKeepAlive() { _ioBuffer.enableKeepAlive(); }
		inline bool isRequireClose() { return (_ioBuffer.isRequireClose() ? true : _ioBuffer.isTransmissionStopped()); }
		inline void setUntransmittedSeconds(int untransmittedSeconds) { _ioBuffer.setUntransmittedSeconds(untransmittedSeconds); }
		void sendCachedData(bool& needWaitSendEvent, bool socketReady = false);
		void sendData(bool& needWaitSendEvent, std::string* data, int64_t expiredMS, bool discardable);
		void sendCloseSignal(bool& needWaitSendEvent)
		{
			_ioBuffer.sendCloseSignal(needWaitSendEvent);
		}

		inline bool getRecvToken() { return _ioBuffer.getRecvToken(); }
		inline void returnRecvToken() { return _ioBuffer.returnRecvToken(); }
		bool recvData(std::list<FPQuestPtr>& questList, std::list<FPAnswerPtr>& answerList);

		virtual void embed_configRecvNotifyDelegate(EmbedRecvNotifyDelegate delegate)
		{
			_ioBuffer.configEmbedInfos(_connectionInfo->uniqueId(), delegate);
		}
	};

	//===============[ UDPClientIOProcessor ]=====================//
	class UDPClientIOProcessor
	{
		static void read(UDPClientConnection * connection);
		static bool deliverAnswer(UDPClientConnection * connection, FPAnswerPtr answer);
		static bool deliverQuest(UDPClientConnection * connection, FPQuestPtr quest);
		static void closeConnection(UDPClientConnection * connection, bool normalClosed);

	public:
		static void processConnectionIO(UDPClientConnection * connection, bool canRead, bool canWrite);
	};
}

#endif