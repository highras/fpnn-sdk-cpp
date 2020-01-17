#include "Decoder.h"
#include "TCPClient.h"
#include "ClientIOWorker.h"
#include "ClientEngine.h"

using namespace fpnn;

bool TCPClientConnection::waitForSendEvent()
{
	return ClientEngine::instance()->waitSendEvent(this);
}

bool TCPClientIOProcessor::read(TCPClientConnection * connection, bool& closed)
{
	if (!connection->_recvBuffer.getToken())
		return true;

	while (true)
	{
		bool needNextEvent;
		if (connection->recvPackage(needNextEvent, closed) == false)
		{
			connection->_recvBuffer.returnToken();
			LOG_ERROR("Error occurred when client receiving. Connection will be closed soon. %s", connection->_connectionInfo->str().c_str());
			return false;
		}
		if (closed)
		{
			connection->_recvBuffer.returnToken();
			return true;
		}

		if (needNextEvent)
		{
			connection->_recvBuffer.returnToken();
			return true;
		}

		FPQuestPtr quest;
		FPAnswerPtr answer;
		bool status = connection->_recvBuffer.fetch(quest, answer);
		if (status == false)
		{
			LOG_ERROR("Client receiving & decoding data error. Connection will be closed soon. %s", connection->_connectionInfo->str().c_str());
			return false;
		}
		if (quest)
		{
			if (deliverQuest(connection, quest) == false)
			{
				connection->_recvBuffer.returnToken();
				return false;
			}
		}
		else
		{
			if (deliverAnswer(connection, answer) == false)
			{
				connection->_recvBuffer.returnToken();
				return false;
			}
		}
	}
	connection->_recvBuffer.returnToken();
	return true;
}

bool TCPClientIOProcessor::deliverAnswer(TCPClientConnection * connection, FPAnswerPtr answer)
{
	TCPClientPtr client = connection->client();
	if (client)
	{
		client->dealAnswer(answer, connection->_connectionInfo);
		return true;
	}
	else
	{
		return false;
	}
}

bool TCPClientIOProcessor::deliverQuest(TCPClientConnection * connection, FPQuestPtr quest)
{
	TCPClientPtr client = connection->client();
	if (client)
	{
		client->dealQuest(quest, connection->_connectionInfo);
		return true;
	}
	else
	{
		LOG_ERROR("Duplex client is destroyed. Connection will be closed. %s", connection->_connectionInfo->str().c_str());
		return false;
	}
}

void TCPClientIOProcessor::processConnectionIO(TCPClientConnection * connection, bool canRead, bool canWrite)
{
	bool closed = false;
	bool fdInvalid = false;
	bool needWaitSendEvent = false;

	if (canRead)
		fdInvalid = !read(connection, closed);

	if (closed)
	{
		closeConnection(connection, true);
		return;
	}

	if (!fdInvalid && canWrite)
	{
		int errno_ = connection->send(needWaitSendEvent);
		switch (errno_)
		{
		case 0:
			break;

		case EPIPE:
		case EBADF:
		case EINVAL:
		default:
			fdInvalid = true;
			LOG_ERROR("Client connection sending error. Connection will be closed soon. %s", connection->_connectionInfo->str().c_str());
		}
	}
	
	if (fdInvalid == false)
	{
		if (needWaitSendEvent)
		{
			if (ClientEngine::instance()->waitSendEvent(connection))
			{
				connection->_refCount--;
				return;
			}
		}
		else
		{
			connection->_refCount--;
			return;
		}

		LOG_INFO("Client connection wait event failed. Connection will be closed. %s", connection->_connectionInfo->str().c_str());
	}

	closeConnection(connection, false);
}

void TCPClientIOProcessor::closeConnection(TCPClientConnection * connection, bool normalClosed)
{
	bool closedByError = !normalClosed;
	int errorCode = normalClosed ? FPNN_EC_CORE_CONNECTION_CLOSED : FPNN_EC_CORE_INVALID_CONNECTION;

	if (ClientEngine::instance()->takeConnection(connection->_connectionInfo.get()) == NULL)
	{
		connection->_refCount--;
		return;
	}

	ClientEngine::instance()->quit(connection);
	//connection->_refCount--;

	TCPClientPtr client = connection->client();
	if (client)
	{
		client->clearConnectionQuestCallbacks(connection, errorCode);
		client->willClosed(connection, closedByError);
	}
	else
	{
		ClientEngine::instance()->clearConnectionQuestCallbacks(connection, errorCode);
		
		CloseErrorTaskPtr task(new CloseErrorTask(connection, closedByError));
		ClientEngine::runTask(task);
	}

	connection->_refCount--;
}
