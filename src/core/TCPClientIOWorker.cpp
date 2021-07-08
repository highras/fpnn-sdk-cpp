#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include "Decoder.h"
#include "TCPClient.h"
#include "TCPClientIOWorker.h"
#include "ClientEngine.h"

using namespace fpnn;

bool TCPClientConnection::waitForSendEvent()
{
	return ClientEngine::instance()->waitSendEvent(this);
}

bool TCPClientConnection::isIPv4Connected()
{
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(_connectionInfo->ip.c_str()); 
	serverAddr.sin_port = htons(_connectionInfo->port);

	if (::connect(_connectionInfo->socket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) != 0)
	{
		if (errno == EISCONN)
			return true;
		
		return false;
	}
	return true;
}

bool TCPClientConnection::isIPv6Connected()
{
	struct sockaddr_in6 serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin6_family = AF_INET6;  
	serverAddr.sin6_port = htons(_connectionInfo->port);

	inet_pton(AF_INET6, _connectionInfo->ip.c_str(), &serverAddr.sin6_addr);

	if (::connect(_connectionInfo->socket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) != 0)
	{
		if (errno == EISCONN)
			return true;
		
		return false;
	}
	return true;
}

bool TCPClientConnection::connectedEventCompleted()
{
	_recvBuffer.allowReceiving();
	_sendBuffer.allowSending();

	bool needWaitSendEvent = false;
	if (send(needWaitSendEvent) == false)
		return false;

	if (needWaitSendEvent)
		return waitForSendEvent();

	return true;
}

bool TCPClientIOProcessor::read(TCPClientConnection * connection, bool& closed)
{
	if (!connection->_recvBuffer.getToken())
		return true;

	connection->updateReceivedMS();

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

		if (connection->_embedRecvNotifyDeleagte == NULL)
		{
			FPQuestPtr quest;
			FPAnswerPtr answer;
			bool status = connection->_recvBuffer.fetch(quest, answer);
			if (status == false)
			{
				connection->_recvBuffer.returnToken();
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
		else
		{
			bool status = connection->_recvBuffer.embed_fetchRawData(
				connection->_connectionInfo->uniqueId(), connection->_embedRecvNotifyDeleagte);
			if (status == false)
			{
				connection->_recvBuffer.returnToken();
				LOG_ERROR("Client receiving data in embedded mode error. Connection will be closed soon. %s", connection->_connectionInfo->str().c_str());
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

void TCPClientIOProcessor::processConnectingOperations(TCPClientConnection * connection)
{
	connection->_socketConnected = true;

	TCPClientPtr client = connection->client();
	if (client)
	{
		client->socketConnected(connection, connection->isConnected());
		connection->_refCount--;
	}
	else
		closeConnection(connection, false);
}

void TCPClientIOProcessor::processConnectionIO(TCPClientConnection * connection, bool canRead, bool canWrite)
{
	if (connection->_socketConnected)
		;
	else
	{
		processConnectingOperations(connection);
		return;
	}

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

		LOG_INFO("TCP connection wait event failed. Connection will be closed. %s", connection->_connectionInfo->str().c_str());
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
		client->willClose(connection, closedByError);
	}
	else
	{
		ClientEngine::instance()->clearConnectionQuestCallbacks(connection, errorCode);
		
		std::shared_ptr<ClientCloseTask> task(new ClientCloseTask(connection->questProcessor(), connection, closedByError));
		ClientEngine::runTask(task);
		ClientEngine::instance()->reclaim(task);
	}

	connection->_refCount--;
}
