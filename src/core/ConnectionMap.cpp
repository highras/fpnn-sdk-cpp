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
			BasicConnection* connection = it->second;
			
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
			BasicConnection* connection = cmp.second;

			std::map<uint32_t, BasicAnswerCallback*> basMap;
			timeouted.push_back(basMap);

			std::map<uint32_t, BasicAnswerCallback*>& currMap = timeouted.back();
			for (auto& cbPair: connection->_callbackMap)
			{
				if (cbPair.second->expiredTime() <= threshold)
					currMap[cbPair.first] = cbPair.second;
			}

			for (auto& bacPair: currMap)
				connection->_callbackMap.erase(bacPair.first);
		}
	}

	void ConnectionMap::extractTimeoutedConnections(int64_t threshold, std::list<BasicConnection*>& timeouted)
	{
		std::list<int> timeoutedNodes;
		std::unique_lock<std::mutex> lck(_mutex);
		for (auto& cmp: _connections)
		{
			BasicConnection* connection = cmp.second;

			if (connection->_activeTime <= threshold)
			{
				timeoutedNodes.push_back(cmp.first);
				timeouted.push_back(connection);
			}
		}

		for (auto n: timeoutedNodes)
			_connections.erase(n);
	}

	bool ConnectionMap::sendQuestWithBasicAnswerCallback(int socket, uint64_t token, FPQuestPtr quest, BasicAnswerCallback* callback, int timeout, bool discardableUDPQuest)
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
		catch (const std::exception& ex)
		{
			LOG_ERROR("Quest Raw Exception: %s", ex.what());
			return false;
		}
		catch (...)
		{
			LOG_ERROR("Quest Raw Exception.");
			return false;
		}

		uint32_t seqNum = quest->seqNumLE();

		if (callback)
			callback->updateExpiredTime(TimeUtil::curr_msec() + timeout);

		bool status = sendQuest(socket, token, raw, seqNum, callback, timeout, discardableUDPQuest);
		if (!status)
			delete raw;

		return status;
	}

	FPAnswerPtr ConnectionMap::sendQuest(int socket, uint64_t token, std::mutex* mutex, FPQuestPtr quest, int timeout, bool discardableUDPQuest)
	{
		if (!quest->isTwoWay())
		{
			sendQuestWithBasicAnswerCallback(socket, token, quest, NULL, 0, discardableUDPQuest);
			return NULL;
		}

		std::shared_ptr<SyncedAnswerCallback> s(new SyncedAnswerCallback(mutex, quest));
		if (!sendQuestWithBasicAnswerCallback(socket, token, quest, s.get(), timeout, discardableUDPQuest))
		{
			return FPAWriter::errorAnswer(quest, FPNN_EC_CORE_SEND_ERROR, "unknown sending error.");
		}

		return s->takeAnswer();
	}

	void ConnectionMap::periodUDPSendingCheck(std::unordered_set<UDPClientConnection*>& invalidOrExpiredConnections)
	{
		std::set<UDPClientConnection*> udpConnections;
		std::unordered_set<UDPClientConnection*> invalidConns;
		{
			std::unique_lock<std::mutex> lck(_mutex);
			for (auto& cmp: _connections)
			{
				BasicConnection* connection = cmp.second;
				if (connection->connectionType() == BasicConnection::UDPClientConnectionType)
				{
					UDPClientConnection* conn = (UDPClientConnection*)connection;
					if (conn->invalid() == false)
					{
						conn->_refCount++;
						udpConnections.insert(conn);
					}
					else
					{
						invalidConns.insert(conn);
						invalidOrExpiredConnections.insert(conn);
					}
				}
			}

			for (UDPClientConnection* conn: invalidConns)
				_connections.erase(conn->_connectionInfo->socket);
		}

		for (auto conn: udpConnections)
		{
			bool needWaitSendEvent = false;
			conn->sendCachedData(needWaitSendEvent, false);
			if (needWaitSendEvent)
				conn->waitForSendEvent();

			conn->_refCount--;
		}
	}

	void ConnectionMap::TCPClientKeepAlive(std::list<TCPClientConnection*>& invalidConnections,
		std::list<TCPClientConnection*>& connectExpiredConnections)
	{
		std::list<TCPClientKeepAliveTimeoutInfo> keepAliveList;

		//-- Step 1: pick invalid connections & requiring ping connections
		{
			bool isLost;
			int timeout;
			std::list<int> invalidSockets;
			int64_t now = slack_real_msec();

			std::unique_lock<std::mutex> lck(_mutex);

			for (auto& cmp: _connections)
			{
				BasicConnection* connection = cmp.second;
				if (connection->connectionType() == BasicConnection::TCPClientConnectionType)
				{
					TCPClientConnection* tcpClientConn = (TCPClientConnection*)connection;

					if (tcpClientConn->_socketConnected)
					{
						//-- Keep alive checking
						timeout = tcpClientConn->isRequireKeepAlive(isLost);
						if (isLost)
						{
							invalidConnections.push_back(tcpClientConn);
							invalidSockets.push_back(cmp.first);
						}
						else if (timeout > 0)
						{
							connection->_refCount++;
							keepAliveList.push_back(TCPClientKeepAliveTimeoutInfo{ tcpClientConn, timeout });
						}
					}
					else if (tcpClientConn->_connectingExpiredMS <= now)	//-- Connecting expired.
					{
						connectExpiredConnections.push_back(tcpClientConn);
						invalidSockets.push_back(cmp.first);
					}
				}
			}

			for (auto s: invalidSockets)
				_connections.erase(s);
		}

		//--  Step 2: send ping
		if (keepAliveList.size() > 0)
		{
			TCPClientSharedKeepAlivePingDatas sharedPing;
			sharedPing.build();
			sendTCPClientKeepAlivePingQuest(sharedPing, keepAliveList);
		}
	}
}
