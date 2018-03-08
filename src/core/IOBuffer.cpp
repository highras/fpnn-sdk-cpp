#include <errno.h>
#include "Endian.h"
#include "IOBuffer.h"

using namespace fpnn;

bool RecvBuffer::entryEncryptMode(uint8_t *key, size_t key_len, uint8_t *iv, bool streamMode)
{
	if (_receivedPackage > 1)
		return false;

	delete _receiver;
	if (streamMode)
		_receiver = new EncryptedStreamReceiver(key, key_len, iv);
	else
		_receiver = new EncryptedPackageReceiver(key, key_len, iv);

	return true;
}

void SendBuffer::encryptData()
{
	if (_sentPackage > 0)
		_encryptor->encrypt(_currBuffer);
	else
	{
		if (!_encryptAfterFirstPackage)
			_encryptor->encrypt(_currBuffer);
	}
}

int SendBuffer::realSend(int fd, bool& needWaitSendEvent)
{
	uint64_t currSendBytes = 0;

	needWaitSendEvent = false;
	while (true)
	{
		if (_currBuffer == NULL)
		{
			CurrBufferProcessFunc currBufferProcess;
			{
				std::unique_lock<std::mutex> lck(*_mutex);
				if (_outQueue.size() == 0)
				{
					_sentBytes += currSendBytes;
					_sendToken = true;
					return 0;
				}

				_currBuffer = _outQueue.front();
				_outQueue.pop();
				_offset = 0;

				currBufferProcess = _currBufferProcess;
			}

			if (currBufferProcess)
				(this->*currBufferProcess)();
		}

		size_t requireSend = _currBuffer->length() - _offset;
		ssize_t sendBytes = write(fd, _currBuffer->data() + _offset, requireSend);
		if (sendBytes == -1)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				needWaitSendEvent = true;
				std::unique_lock<std::mutex> lck(*_mutex);
				_sentBytes += currSendBytes;
				_sendToken = true;
				return 0;
			}
			if (errno == EINTR)
				continue;

			std::unique_lock<std::mutex> lck(*_mutex);
			_sentBytes += currSendBytes;
			_sendToken = true;
			return errno;
		}
		else
		{
			_offset += (size_t)sendBytes;
			currSendBytes += (uint64_t)sendBytes;
			if (_offset == _currBuffer->length())
			{
				delete _currBuffer;
				_currBuffer = NULL;
				_offset = 0;
				_sentPackage += 1;
			}
		}
	}
}

int SendBuffer::send(int fd, bool& needWaitSendEvent, std::string* data)
{
	if (data && data->empty())
	{
		delete data;
		data = NULL;
	}

	{
		std::unique_lock<std::mutex> lck(*_mutex);
		if (data)
			_outQueue.push(data);

		if (!_sendToken)
			return 0;

		_sendToken = false;
	}

	//-- Token will be return in realSend() function.
	int err = realSend(fd, needWaitSendEvent); 	//-- ignore all error status. it will be deal in IO thread.
	return err;
}

bool SendBuffer::entryEncryptMode(uint8_t *key, size_t key_len, uint8_t *iv, bool streamMode)
{
	if (_encryptor)
		return false;

	Encryptor* encryptor = NULL;
	if (streamMode)
		encryptor = new StreamEncryptor(key, key_len, iv);
	else
		encryptor = new PackageEncryptor(key, key_len, iv);

	{
		std::unique_lock<std::mutex> lck(*_mutex);
		if (_sentBytes) return false;
		if (_sendToken == false) return false;

		_encryptor = encryptor;
		_currBufferProcess = &SendBuffer::encryptData;
	}

	return true;
}

void SendBuffer::appendData(std::string* data)
{
	if (data && data->empty())
	{
		delete data;
		return;
	}

	std::unique_lock<std::mutex> lck(*_mutex);
	if (data)
		_outQueue.push(data);
}
