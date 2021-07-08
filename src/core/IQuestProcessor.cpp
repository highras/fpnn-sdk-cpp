#include "ClientEngine.h"
#include "AdvanceAnswer.h"
#include "IQuestProcessor.h"

using namespace fpnn;

std::atomic<uint64_t> ConnectionInfo::uniqueIdBase(0);

struct AnswerStatus
{
	bool _answered;
	FPQuestPtr _quest;
	ConnectionInfoPtr _connInfo;

	AnswerStatus(ConnectionInfoPtr connInfo, FPQuestPtr quest): _answered(false), _quest(quest), _connInfo(connInfo) {}
};

static thread_local std::unique_ptr<AnswerStatus> gtl_answerStatus;

void IQuestProcessor::initAnswerStatus(ConnectionInfoPtr connInfo, FPQuestPtr quest)
{
	gtl_answerStatus.reset(new AnswerStatus(connInfo, quest));
}

bool IQuestProcessor::getQuestAnsweredStatus()
{
	return gtl_answerStatus->_answered;
}

bool IQuestProcessor::finishAnswerStatus()
{
	bool status = gtl_answerStatus->_answered;
	gtl_answerStatus = nullptr;
	return status;
}

bool IQuestProcessor::sendAnswer(FPAnswerPtr answer)
{
	if (!answer || !gtl_answerStatus)
		return false;

	if (gtl_answerStatus->_answered)
		return false;

	if (!gtl_answerStatus->_quest->isTwoWay())
		return false;

	std::string* raw = answer->raw();

	ConnectionInfoPtr connInfo = gtl_answerStatus->_connInfo;

	if (connInfo->isTCP())
		_concurrentSender->sendTCPData(connInfo->socket, connInfo->token, raw);
	else
	{
		//int64_t expiredMS = ClientEngine::instance()->getQuestTimeout() * 1000;
		_concurrentSender->sendUDPData(connInfo->socket, connInfo->token, raw, 0/*slack_real_msec() + expiredMS*/, false);
	}

	gtl_answerStatus->_answered = true;
	return true;
}

IAsyncAnswerPtr IQuestProcessor::genAsyncAnswer()
{
	if (!gtl_answerStatus)
		return nullptr;

	if (gtl_answerStatus->_answered)
		return nullptr;

	if (gtl_answerStatus->_quest->isOneWay())
		return nullptr;

	IAsyncAnswerPtr async(new AsyncAnswerImp(_concurrentSender, gtl_answerStatus->_connInfo, gtl_answerStatus->_quest));
	gtl_answerStatus->_answered = true;
	return async;
}

FPAnswerPtr IQuestProcessor::sendQuest(FPQuestPtr quest, int timeout)
{
	return sendQuestEx(quest, quest->isOneWay(), timeout * 1000);
}

bool IQuestProcessor::sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout)
{
	return sendQuestEx(quest, callback, quest->isOneWay(), timeout * 1000);
}

bool IQuestProcessor::sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout)
{
	return sendQuestEx(quest, std::move(task), quest->isOneWay(), timeout * 1000);
}

FPAnswerPtr IQuestProcessor::sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec)
{
	if (!gtl_answerStatus)
	{
		if (quest->isTwoWay())
			return FPAWriter::errorAnswer(quest, FPNN_EC_CORE_FORBIDDEN, "Please call this method in the duplex thread.");
		else
			return nullptr;
	}

	ConnectionInfoPtr connInfo = gtl_answerStatus->_connInfo;

	if (connInfo->isTCP())
		return _concurrentSender->sendQuest(connInfo->socket, connInfo->token, connInfo->_mutex, quest, timeoutMsec);
	else
	{
		return ClientEngine::instance()->sendQuest(connInfo->socket,
			connInfo->token, connInfo->_mutex, quest, timeoutMsec, discardable);
	}
}
bool IQuestProcessor::sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec)
{
	if (!gtl_answerStatus)
		return false;

	ConnectionInfoPtr connInfo = gtl_answerStatus->_connInfo;

	if (connInfo->isTCP())
		return _concurrentSender->sendQuest(connInfo->socket, connInfo->token, quest, callback, timeoutMsec);
	else
	{
		return ClientEngine::instance()->sendQuest(connInfo->socket,
			connInfo->token, quest, callback, timeoutMsec, discardable);
	}
}
bool IQuestProcessor::sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec)
{
	if (!gtl_answerStatus)
		return false;

	ConnectionInfoPtr connInfo = gtl_answerStatus->_connInfo;

	if (connInfo->isTCP())
		return _concurrentSender->sendQuest(connInfo->socket, connInfo->token, quest, std::move(task), timeoutMsec);
	else
	{
		return ClientEngine::instance()->sendQuest(connInfo->socket,
			connInfo->token, quest, std::move(task), timeoutMsec, discardable);
	}
}


FPAnswerPtr QuestSender::sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec)
{
	return ClientEngine::instance()->sendQuest(_socket, _token, _mutex, quest, timeoutMsec, discardable);
}
bool QuestSender::sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec)
{
	return ClientEngine::instance()->sendQuest(_socket, _token, quest, callback, timeoutMsec, discardable);
}
bool QuestSender::sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec)
{
	return ClientEngine::instance()->sendQuest(_socket, _token, quest, std::move(task), timeoutMsec, discardable);
}
