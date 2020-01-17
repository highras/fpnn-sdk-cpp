#ifndef FPNN_Client_IO_Worker_H
#define FPNN_Client_IO_Worker_H

#include <time.h>
#include "IQuestProcessor.h"
#include "IOBuffer.h"

namespace fpnn
{
	class TCPClient;
	typedef std::shared_ptr<TCPClient> TCPClientPtr;

	class IReleaseable;
	typedef std::shared_ptr<IReleaseable> IReleaseablePtr;

	class TCPClientConnection;
	typedef std::shared_ptr<TCPClientConnection> TCPClientConnectionPtr;

	//===============[ IReleaseable ]=====================//
	class IReleaseable
	{
	public:
		virtual bool releaseable() = 0;
		virtual ~IReleaseable() {}
	};

	//===============[ TCPClientConnection ]=====================//
	class TCPClientConnection: public IReleaseable
	{
	private:
		std::weak_ptr<TCPClient> _client;

	public:
		ConnectionInfoPtr _connectionInfo;
		
		int64_t _activeTime;
		std::atomic<int> _refCount;
		RecvBuffer _recvBuffer;
		SendBuffer _sendBuffer;

		std::unordered_map<uint32_t, BasicAnswerCallback*> _callbackMap;
		IQuestProcessorPtr _questProcessor;

	public:
		virtual bool waitForSendEvent();
		TCPClientPtr client() { return _client.lock(); }
		virtual bool releaseable() { return (_refCount == 0); }

		inline bool recvPackage(bool& needNextEvent, bool& closed)
		{
			bool rev = _recvBuffer.recvPackage(_connectionInfo->socket, needNextEvent);
			closed = _recvBuffer.isClosed();
			return rev;
		}
		inline bool entryEncryptMode(uint8_t *key, size_t key_len, uint8_t *iv, bool streamMode)
		{
			if (_recvBuffer.entryEncryptMode(key, key_len, iv, streamMode) == false)
			{
				LOG_ERROR("Entry encrypt mode failed. Entry cmd is not the first cmd. Connection will be closed by server. %s", _connectionInfo->str().c_str());
				return false;
			}
			if (_sendBuffer.entryEncryptMode(key, key_len, iv, streamMode) == false)
			{
				LOG_ERROR("Entry encrypt mode failed. Connection has bytes sending. Connection will be closed by server. %s", _connectionInfo->str().c_str());
				return false;
			}
			_connectionInfo->_encrypted = true;
			return true;
		}
		inline void encryptAfterFirstPackageSent() { _sendBuffer.encryptAfterFirstPackage(); }
		int socket() const { return _connectionInfo->socket; }

		inline bool isEncrypted() { return _connectionInfo->_encrypted; }
		inline int send(bool& needWaitSendEvent, std::string* data = NULL)
		{
			//-- _activeTime vaule maybe in confusion after concurrent Sending on one connection.
			//-- But the probability is very low even server with high load. So, it hasn't be adjusted at current.
			_activeTime = time(NULL);
			return _sendBuffer.send(_connectionInfo->socket, needWaitSendEvent, data);
		}
		
		TCPClientConnection(TCPClientPtr client, std::mutex* mutex,
			ConnectionInfoPtr connectionInfo, IQuestProcessorPtr questProcessor):
			_client(client), _connectionInfo(connectionInfo),
			_refCount(0), _recvBuffer(mutex), _sendBuffer(mutex),
			_questProcessor(questProcessor)
		{
			_connectionInfo->token = (uint64_t)this;	//-- if use Virtual Derive, must redo this in subclass constructor.
			_connectionInfo->_mutex = mutex;
			_activeTime = time(NULL);
		}

		virtual ~TCPClientConnection()
		{
			close(_connectionInfo->socket);
		}
	};

	//===============[ TCPClientIOProcessor ]=====================//
	class TCPClientIOProcessor
	{
		static bool read(TCPClientConnection * connection, bool& closed);
		static bool deliverAnswer(TCPClientConnection * connection, FPAnswerPtr answer);
		static bool deliverQuest(TCPClientConnection * connection, FPQuestPtr quest);
		static void closeConnection(TCPClientConnection * connection, bool normalClosed);

	public:
		static void processConnectionIO(TCPClientConnection * connection, bool canRead, bool canWrite);
	};
}

#endif
