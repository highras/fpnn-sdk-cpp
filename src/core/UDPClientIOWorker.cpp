#include <time.h>
#include "ClientEngine.h"
#include "UDPClientIOWorker.h"
#include "UDPClient.h"

using namespace fpnn;

//====================================================//
//--                 UDPClientConnection                  --//
//====================================================//
bool UDPClientConnection::waitForSendEvent()
{
	return ClientEngine::instance()->waitSendEvent(this);
}

void UDPClientConnection::sendCachedData(bool& needWaitSendEvent, bool socketReady)
{
	bool blockByFlowControl = false;
	_ioBuffer.sendCachedData(needWaitSendEvent, blockByFlowControl, socketReady);
	_activeTime = time(NULL);
}
void UDPClientConnection::sendData(bool& needWaitSendEvent, std::string* data, int64_t expiredMS, bool discardable)
{
	bool blockByFlowControl = false;
	_ioBuffer.sendData(needWaitSendEvent, blockByFlowControl, data, expiredMS, discardable);
	_activeTime = time(NULL);
}

bool UDPClientConnection::recvData(std::list<FPQuestPtr>& questList, std::list<FPAnswerPtr>& answerList)
{
	bool status = _ioBuffer.recvData();
	questList.swap(_ioBuffer.getReceivedQuestList());
	answerList.swap(_ioBuffer.getReceivedAnswerList());

	/*
	*  目前 _activeTime 没有使用。而且 _activeTime 仅对非 embed 模式生效。
	*  如果 embed 模式需要使用 _activeTime，则需要修改下述代码，增加 embed 的处理部分。
	*/
	if (questList.size() || answerList.size())
		_activeTime = time(NULL);

	return status;
}

//====================================================//
//--              UDPClientIOWorker                 --//
//====================================================//
void UDPClientIOProcessor::read(UDPClientConnection * connection)
{
	if (!connection->getRecvToken())
		return;
	
	std::list<FPQuestPtr> questList;
	std::list<FPAnswerPtr> answerList;
	
	bool goon = true;
	while (goon && connection->isRequireClose() == false)
	{
		goon = connection->recvData(questList, answerList);

		for (auto& answer: answerList)
			if (!deliverAnswer(connection, answer))
				break;
		
		for (auto& quest: questList)
			if (!deliverQuest(connection, quest))
				break;

		questList.clear();
		answerList.clear();
	}

	connection->returnRecvToken();
}

bool UDPClientIOProcessor::deliverAnswer(UDPClientConnection * connection, FPAnswerPtr answer)
{
	UDPClientPtr client = connection->client();
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

bool UDPClientIOProcessor::deliverQuest(UDPClientConnection * connection, FPQuestPtr quest)
{
	UDPClientPtr client = connection->client();
	if (client)
	{
		client->dealQuest(quest, connection->_connectionInfo);
		return true;
	}
	else
	{
		LOG_ERROR("UDP duplex client is destroyed. Connection will be closed. %s", connection->_connectionInfo->str().c_str());
		return false;
	}
}

void UDPClientIOProcessor::processConnectionIO(UDPClientConnection * connection, bool canRead, bool canWrite)
{
	if (canRead)
		read(connection);

	bool needWaitSendEvent = false;
	//-- 如果有 read 事件，强制调用 send 检查是否有数据需要发送（大概率有ARQ应答包需要发送）。
	connection->sendCachedData(needWaitSendEvent, true);

	if (connection->isRequireClose())
	{
		closeConnection(connection, true);
		return;
	}

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

		LOG_INFO("UDP connection wait event failed. System memory maybe run out. Connection maybe unusable. %s", connection->_connectionInfo->str().c_str());
	}

	closeConnection(connection, false);
}

void UDPClientIOProcessor::closeConnection(UDPClientConnection * connection, bool normalClosed)
{
	bool closedByError = !normalClosed;
	int errorCode = normalClosed ? FPNN_EC_CORE_CONNECTION_CLOSED : FPNN_EC_CORE_INVALID_CONNECTION;

	if (ClientEngine::instance()->takeConnection(connection->socket()) == NULL)
	{
		connection->_refCount--;
		return;
	}

	ClientEngine::instance()->quit(connection);
	// connection->_refCount--;

	UDPClientPtr client = connection->client();
	if (client)
	{
		client->clearConnectionQuestCallbacks(connection, errorCode);
		client->willClose(connection, errorCode);
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
