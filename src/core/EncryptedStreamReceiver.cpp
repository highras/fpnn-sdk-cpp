#ifdef __APPLE__
	#include <sys/types.h>
	#include <sys/uio.h>
#endif
#include <unistd.h>
#include "FPLog.h"
#include "Config.h"
#include "Decoder.h"
#include "Receiver.h"

using namespace fpnn;

int EncryptedStreamReceiver::remainDataLen()
{
	//-- check Magic Header
	if (FPMessage::isTCP((char *)_decHeader))
		return (int)(sizeof(FPMessage::Header) + FPMessage::BodyLen((char *)_decHeader)) - _curr;
	else
		return -1;
}

bool EncryptedStreamReceiver::recv(int fd, int length)
{
	if (length)
	{
		_total += length;
		if (_total > Config::_max_recv_package_length)
		{
			LOG_ERROR("Recv huge TCP data from socket: %d. Connection will be closed by framework.", fd);
			return false;
		}
	}

	while (_curr < _total)
	{
		int requireRead = _total - _curr;
		int readBytes = (int)::read(fd, _currBuf + _curr, requireRead);
		if (readBytes != requireRead)
		{
			if (readBytes == 0)
			{
				_closed = true;
				return (_curr == 0);
			}

			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				if (readBytes > 0)
					_curr += readBytes;

				return true;
			}

			if (errno == 0 || errno == EINTR)
			{
				if (readBytes > 0)
				{
					_curr += readBytes;
					continue;
				}	
				else
					return false;
			}
			else
				return false;
		}
		
		_curr += readBytes;
		return true;
	}
	return true;
}

bool EncryptedStreamReceiver::recvTcpPackage(int fd, int length, bool& needNextEvent)
{
	if (recv(fd, length) == false)
		return false;

	if (_closed)
		return true;

	needNextEvent = (_curr < _total);
	return true;
}

bool EncryptedStreamReceiver::recvPackage(int fd, bool& needNextEvent)
{
	if (_curr < FPMessage::_HeaderLength)
	{
		if (recv(fd) == false)
			return false;

		if (_closed)
			return true;

		if (_curr < FPMessage::_HeaderLength)
		{
			needNextEvent = true;
			return true;
		}
	}

	int length = 0;
	if (_curr == _total && _total == FPMessage::_HeaderLength)
	{
		_encryptor.decrypt(_decHeader, _header, FPMessage::_HeaderLength);
		length = remainDataLen();

		if (length > 0)
		{
			if (_bodyBuffer)
				free(_bodyBuffer);
			
			_bodyBuffer = (uint8_t*)malloc(length + FPMessage::_HeaderLength);
			_currBuf = _bodyBuffer;
		}
		else
		{
			LOG_ERROR("Received Error data (Not available FPNN-TCP-Message), fd:%d", fd);
			return false;
		}
	}

	return recvTcpPackage(fd, length, needNextEvent);
}

bool EncryptedStreamReceiver::fetch(FPQuestPtr& quest, FPAnswerPtr& answer)
{
	if (_curr != _total)
		return false;

	int dataLen = _total;
	char* buf = (char*)malloc(dataLen);
	memcpy(buf, _decHeader, FPMessage::_HeaderLength);
	_encryptor.decrypt((uint8_t *)buf + FPMessage::_HeaderLength, _bodyBuffer + FPMessage::_HeaderLength, dataLen - FPMessage::_HeaderLength);

	free(_bodyBuffer);
	_bodyBuffer = NULL;

	_currBuf = _header;
	_curr = 0;
	_total = FPMessage::_HeaderLength;

	//------- begin decode -------//
	bool rev = false;
	const char *desc = "unknown";
	try
	{
		if (FPMessage::isQuest(buf))
		{
			desc = "TCP quest";
			quest = Decoder::decodeQuest(buf, dataLen);
		}
		else
		{
			desc = "TCP answer";
			answer = Decoder::decodeAnswer(buf, dataLen);
		}
		rev = true;
	}
	catch (const FpnnError& ex)
	{
		LOG_ERROR("Decode %s error. Connection will be closed by server. Code: %d, error: %s.", desc, ex.code(), ex.what());
	}
	catch (...)
	{
		LOG_ERROR("Decode %s error. Connection will be closed by server.", desc);
	}

	free(buf);
	return rev;
}
