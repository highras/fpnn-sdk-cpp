#ifndef FPNN_Receiver_h
#define FPNN_Receiver_h

/*
	!!! IMPORTANT !!!

	*. all fetch(bool &isQuest) function:
		MUST call it only when a package received completed.
*/

#include "Encryptor.h"
#include "FPMessage.h"

namespace fpnn
{
	//================================//
	//--     Receiver Interface     --//
	//================================//
	class Receiver
	{
	protected:
		int _curr;
		int _total;
		bool _closed;

	public:
		Receiver(): _curr(0), _total(FPMessage::_HeaderLength), _closed(false) {}
		virtual ~Receiver() {}

		virtual bool isClosed() { return _closed; }
		virtual bool recvPackage(int fd, bool& needNextEvent) = 0;
		virtual bool fetch(FPQuestPtr& quest, FPAnswerPtr& answer) = 0;
	};

	//================================//
	//--    UnencryptedReceiver     --//
	//================================//
	class UnencryptedReceiver: public Receiver
	{
		uint8_t* _header;
		uint8_t* _currBuf;
		uint8_t* _bodyBuffer;

		/**
			Only called when protocol is TCP, and header has be read.
			if return -1, mean unsupported protocol.
		*/
		int remainDataLen();

		/** If length > 0; will do _total += length; */
		bool recv(int fd, int length = 0);
		bool recvTcpPackage(int fd, int length, bool& needNextEvent);
		
	public:
		UnencryptedReceiver(): Receiver(), _bodyBuffer(NULL)
		{
			_header = (uint8_t*)malloc(FPMessage::_HeaderLength);
			_currBuf = _header;
		}
		virtual ~UnencryptedReceiver()
		{
			if (_bodyBuffer)
				free(_bodyBuffer);

			free(_header);
		}

		virtual bool recvPackage(int fd, bool& needNextEvent);
		virtual bool fetch(FPQuestPtr& quest, FPAnswerPtr& answer);
	};

	//================================//
	//--  EncryptedStreamReceiver   --//
	//================================//
	class EncryptedStreamReceiver: public Receiver
	{
		StreamEncryptor _encryptor;
		uint8_t* _header;
		uint8_t* _decHeader;
		uint8_t* _currBuf;
		uint8_t* _bodyBuffer;

		/**
			Only called when protocol is TCP, and header has be read.
			if return -1, mean unsupported protocol.
		*/
		int remainDataLen();

		/** If length > 0; will do _total += length; */
		bool recv(int fd, int length = 0);
		bool recvTcpPackage(int fd, int length, bool& needNextEvent);
		
	public:
		EncryptedStreamReceiver(uint8_t *key, size_t key_len, uint8_t *iv): Receiver(), _encryptor(key, key_len, iv), _bodyBuffer(NULL)
		{
			_header = (uint8_t*)malloc(FPMessage::_HeaderLength);
			_decHeader = (uint8_t*)malloc(FPMessage::_HeaderLength);
			_currBuf = _header;
		}
		virtual ~EncryptedStreamReceiver()
		{
			if (_bodyBuffer)
				free(_bodyBuffer);

			free(_header);
			free(_decHeader);
		}

		virtual bool recvPackage(int fd, bool& needNextEvent);
		virtual bool fetch(FPQuestPtr& quest, FPAnswerPtr& answer);
	};

	//================================//
	//--  EncryptedPackageReceiver  --//
	//================================//
	class EncryptedPackageReceiver: public Receiver
	{
		PackageEncryptor _encryptor;
		uint32_t _packageLen;
		uint8_t* _dataBuffer;
		uint8_t* _currBuf;
		bool _getLength;

		/** If length > 0; will do _total += length; */
		bool recv(int fd, int length = 0);

	public:
		EncryptedPackageReceiver(uint8_t *key, size_t key_len, uint8_t *iv):
			Receiver(), _encryptor(key, key_len, iv), _packageLen(0), _dataBuffer(NULL), _getLength(false)
		{
			_total = sizeof(uint32_t);
			_currBuf = (uint8_t*)&_packageLen;
		}
		virtual ~EncryptedPackageReceiver()
		{
			if (_dataBuffer)
				free(_dataBuffer);
		}

		virtual bool recvPackage(int fd, bool& needNextEvent);
		virtual bool fetch(FPQuestPtr& quest, FPAnswerPtr& answer);
	};
}

#endif
