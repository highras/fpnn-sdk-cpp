#ifdef __APPLE__
	#include <sys/types.h>
	#include <sys/uio.h>
#endif
#include <unistd.h>
#include "Endian.h"
#include "FPLog.h"
#include "Config.h"
#include "Decoder.h"
#include "Receiver.h"

using namespace fpnn;

bool EncryptedPackageReceiver::recv(int fd, int length)
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

			if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ETIMEDOUT)
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

bool EncryptedPackageReceiver::recvPackage(int fd, bool& needNextEvent)
{
	if (_curr < _total)
	{
		if (recv(fd) == false)
			return false;

		if (_closed)
			return true;

		if (_curr < _total)
		{
			needNextEvent = true;
			return true;
		}
	}

	if (_getLength == false)
	{
		uint32_t len = le32toh(_packageLen);
		_packageLen = len;
		_curr = 0;
		_total = len;
		_getLength = true;

		if (_dataBuffer)
			free(_dataBuffer);

		_dataBuffer = (uint8_t*)malloc(_total);
		_currBuf = _dataBuffer;

		return recvPackage(fd, needNextEvent);
	}
	else
	{
		needNextEvent = false;
		return true;
	}
}

bool EncryptedPackageReceiver::fetch(FPQuestPtr& quest, FPAnswerPtr& answer)
{
	if (_curr != _total)
		return false;

	int dataLen = _total;
	char* buf = (char*)malloc(dataLen);
	_encryptor.decrypt((uint8_t *)buf, _dataBuffer, dataLen);

	free(_dataBuffer);
	_dataBuffer = NULL;

	_curr = 0;
	_total = sizeof(uint32_t);
	_getLength = false;
	_currBuf = (uint8_t*)&_packageLen;

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

bool EncryptedPackageReceiver::embed_fetchRawData(TCPClientConnection * connection, EmbedInteriorRecvNotifyDelegate delegate)
{
	if (_curr != _total)
		return false;

	int dataLen = _total;
	char* buf = (char*)malloc(dataLen);
	_encryptor.decrypt((uint8_t *)buf, _dataBuffer, dataLen);

	free(_dataBuffer);
	_dataBuffer = NULL;

	_curr = 0;
	_total = sizeof(uint32_t);
	_getLength = false;
	_currBuf = (uint8_t*)&_packageLen;

	bool status = delegate(connection, buf, dataLen);

	if (Config::_embed_receiveBuffer_freeBySDK)
		free(buf);
	
	return status;
}
