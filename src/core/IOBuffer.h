#ifndef FPNN_IO_Buffer_H
#define FPNN_IO_Buffer_H

#include <unistd.h>
#include <atomic>
#include <string>
#include <queue>
#include <mutex>
#include <memory>
#include "FPMessage.h"
#include "Receiver.h"

namespace fpnn
{
	class RecvBuffer
	{
		std::mutex* _mutex;		//-- only using for sendBuffer and sendToken
		bool _token;
		uint32_t _receivedPackage;
		Receiver* _receiver;

	public:
		RecvBuffer(std::mutex* mutex): _mutex(mutex), _token(true), _receivedPackage(0)
		{
			_receiver = new UnencryptedReceiver();
		}
		~RecvBuffer()
		{
			if (_receiver)
				delete _receiver;
		}

		inline bool isClosed() { return _receiver->isClosed(); }
		inline bool recvPackage(int fd, bool& needNextEvent)
		{
			return _receiver->recvPackage(fd, needNextEvent);
		}
		inline bool fetch(FPQuestPtr& quest, FPAnswerPtr& answer)
		{
			bool rev = _receiver->fetch(quest, answer);
			if (rev) _receivedPackage += 1;
			return rev;
		}
		inline bool getToken()
		{
			std::unique_lock<std::mutex> lck(*_mutex);
			if (!_token)
				return false;

			_token = false;
			return true;
		}
		inline void returnToken()
		{
			std::unique_lock<std::mutex> lck(*_mutex);
			_token = true;
		}

		bool entryEncryptMode(uint8_t *key, size_t key_len, uint8_t *iv, bool streamMode);
	};

	class SendBuffer
	{
		typedef void (SendBuffer::* CurrBufferProcessFunc)();

	private:
		std::mutex* _mutex;		//-- only using for sendBuffer and sendToken
		bool _sendToken;

		size_t _offset;
		std::string* _currBuffer;
		std::queue<std::string*> _outQueue;
		uint64_t _sentBytes;		//-- Total Bytes
		uint64_t _sentPackage;
		bool _encryptAfterFirstPackage;
		Encryptor* _encryptor;

		CurrBufferProcessFunc _currBufferProcess;

		void encryptData();
		int realSend(int fd, bool& needWaitSendEvent);

	public:
		SendBuffer(std::mutex* mutex): _mutex(mutex), _sendToken(true), _offset(0), _currBuffer(0),
			_sentBytes(0), _sentPackage(0), _encryptAfterFirstPackage(false), _encryptor(NULL), _currBufferProcess(NULL) {}
		~SendBuffer()
		{
			while (_outQueue.size())
			{
				std::string* data = _outQueue.front();
				_outQueue.pop();
				delete data;
			}

			if (_currBuffer)
				delete _currBuffer;

			if (_encryptor)
				delete _encryptor;
		}

		/** returned INT: id 0, success, else, is errno. */
		int send(int fd, bool& needWaitSendEvent, std::string* data = NULL);
		bool entryEncryptMode(uint8_t *key, size_t key_len, uint8_t *iv, bool streamMode);
		void encryptAfterFirstPackage() { _encryptAfterFirstPackage = true; }
		void appendData(std::string* data);
	};
}

#endif
