#include "TimeUtil.h"
#include "ConnectionMap.h"

namespace fpnn
{		
	BasicAnswerCallback* ConnectionMap::takeCallback(int socket, uint32_t seqNum)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		auto it = _connections.find(socket);
		if (it != _connections.end())
		{
			TCPClientConnection* connection = it->second;
			
			auto iter = connection->_callbackMap.find(seqNum);
  			if (iter != connection->_callbackMap.end())
  			{
  				BasicAnswerCallback* cb = iter->second;
  				connection->_callbackMap.erase(seqNum);
  				return cb;
  			}
  			return NULL;
		}
		return NULL;
	}

	void ConnectionMap::extractTimeoutedCallback(int64_t threshold, std::list<std::map<uint32_t, BasicAnswerCallback*> >& timeouted)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		for (auto& cmp: _connections)
		{
			TCPClientConnection* connection = cmp.second;

			std::map<uint32_t, BasicAnswerCallback*> basMap;
			timeouted.push_back(basMap);

			std::map<uint32_t, BasicAnswerCallback*>& currMap = timeouted.back();
			for (auto& cbPair: connection->_callbackMap)
			{
				if (cbPair.second->sendTime() <= threshold)
					currMap[cbPair.first] = cbPair.second;
			}

			for (auto& bacPair: currMap)
				connection->_callbackMap.erase(bacPair.first);
		}
	}

	void ConnectionMap::extractTimeoutedConnections(int64_t threshold, std::list<TCPClientConnection*>& timeouted)
	{
		std::list<int> timeoutedNodes;
		std::unique_lock<std::mutex> lck(_mutex);
		for (auto& cmp: _connections)
		{
			TCPClientConnection* connection = cmp.second;

			if (connection->_activeTime <= threshold)
			{
				timeoutedNodes.push_back(cmp.first);
				timeouted.push_back(connection);
			}
		}

		for (auto n: timeoutedNodes)
			_connections.erase(n);
	}

	bool ConnectionMap::sendQuestWithBasicAnswerCallback(int socket, uint64_t token, FPQuestPtr quest, BasicAnswerCallback* callback, int timeout)
	{
		if (!quest)
			return false;

		if (quest->isTwoWay() && !callback)
			return false;

		std::string* raw = NULL;
		try
		{
			raw = quest->raw();
		}
		catch (const FpnnError& ex){
			LOG_ERROR("Quest Raw Exception:(%d)%s", ex.code(), ex.what());
			return false;
		}
		catch (...)
		{
			LOG_ERROR("Quest Raw Exception.");
			return false;
		}

		uint32_t seqNum = quest->seqNumLE();

		if (callback)
			callback->updateSendTime(TimeUtil::curr_msec() + timeout);

		bool status = sendQuest(socket, token, raw, seqNum, callback);
		if (!status)
			delete raw;

		return status;
	}

	FPAnswerPtr ConnectionMap::sendQuest(int socket, uint64_t token, std::mutex* mutex, FPQuestPtr quest, int timeout)
	{
		if (!quest->isTwoWay())
		{
			sendQuestWithBasicAnswerCallback(socket, token, quest, NULL, 0);
			return NULL;
		}

		std::shared_ptr<SyncedAnswerCallback> s(new SyncedAnswerCallback(mutex, quest));
		if (!sendQuestWithBasicAnswerCallback(socket, token, quest, s.get(), timeout))
		{
			return FPAWriter::errorAnswer(quest, FPNN_EC_CORE_SEND_ERROR, "unknown sending error.");
		}

		return s->takeAnswer();
	}
}
