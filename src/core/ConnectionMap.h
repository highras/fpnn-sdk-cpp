#ifndef FPNN_Partitioned_Connection_Map_H
#define FPNN_Partitioned_Connection_Map_H

#include <map>
#include <set>
#include <list>
#include <mutex>
#include <vector>
#include <unordered_map>
#include "FPMessage.h"
#include "FPWriter.h"
#include "AnswerCallbacks.h"
#include "ClientIOWorker.h"

namespace fpnn
{
	class ConnectionMap
	{
		std::mutex _mutex;
		std::unordered_map<int, TCPClientConnection*> _connections;

		inline bool sendData(TCPClientConnection* conn, std::string* data)
		{
			bool needWaitSendEvent = false;
			conn->send(needWaitSendEvent, data);
			if (needWaitSendEvent)
				conn->waitForSendEvent();

			return true;
		}

		inline bool sendQuest(TCPClientConnection* conn, std::string* data, uint32_t seqNum, BasicAnswerCallback* callback)
		{
			if (callback)
				conn->_callbackMap[seqNum] = callback;

			bool status = sendData(conn, data);
			if (!status && callback)
				conn->_callbackMap.erase(seqNum);
			
			return status;
		}

		bool sendQuestWithBasicAnswerCallback(int socket, uint64_t token, FPQuestPtr quest, BasicAnswerCallback* callback, int timeout);

	public:
		TCPClientConnection* takeConnection(int fd)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			auto it = _connections.find(fd);
			if (it != _connections.end())
			{
				TCPClientConnection* conn = it->second;
				_connections.erase(it);
				return conn;
			}
			return NULL;
		}

		TCPClientConnection* takeConnection(const ConnectionInfo* ci)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			auto it = _connections.find(ci->socket);
			if (it != _connections.end())
			{
				TCPClientConnection* conn = it->second;
				if ((uint64_t)conn == ci->token)
				{
					_connections.erase(it);
					return conn;
				}
			}
			return NULL;
		}

		bool insert(int fd, TCPClientConnection* connection)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			auto it = _connections.find(fd);
			if (it == _connections.end())
			{
				_connections[fd] = connection;
				return true;
			}
			return false;
		}

		void remove(int fd)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			_connections.erase(fd);
		}

		TCPClientConnection* signConnection(int fd)
		{
			TCPClientConnection* connection = NULL;
			std::unique_lock<std::mutex> lck(_mutex);
			auto it = _connections.find(fd);
			if (it != _connections.end())
			{
				connection = it->second;
				connection->_refCount++;
			}
			return connection;
		}

		void waitForEmpty()
		{
			while (_connections.size() > 0)
				usleep(20000);
		}

		void getAllSocket(std::set<int>& fdSet)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			for (auto& cmp: _connections)
				fdSet.insert(cmp.first);
		}

		bool sendData(int socket, uint64_t token, std::string* data)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			auto it = _connections.find(socket);
			if (it != _connections.end())
			{
				TCPClientConnection* connection = it->second;
				if (token == (uint64_t)connection)
					return sendData(connection, data);
			}
			return false;
		}

	protected:
		bool sendQuest(int socket, uint64_t token, std::string* data, uint32_t seqNum, BasicAnswerCallback* callback)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			auto it = _connections.find(socket);
			if (it != _connections.end())
			{
				TCPClientConnection* connection = it->second;
				if (token == (uint64_t)connection)
					return sendQuest(connection, data, seqNum, callback);
			}
			return false;
		}

	public:
		BasicAnswerCallback* takeCallback(int socket, uint32_t seqNum);
		void extractTimeoutedCallback(int64_t threshold, std::list<std::map<uint32_t, BasicAnswerCallback*> >& timeouted);
		void extractTimeoutedConnections(int64_t threshold, std::list<TCPClientConnection*>& timeouted);

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.
		*/
		FPAnswerPtr sendQuest(int socket, uint64_t token, std::mutex* mutex, FPQuestPtr quest, int timeout);

		inline bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, AnswerCallback* callback, int timeout)
		{
			return sendQuestWithBasicAnswerCallback(socket, token, quest, callback, timeout);
		}
		inline bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout)
		{
			BasicAnswerCallback* t = new FunctionAnswerCallback(std::move(task));
			if (sendQuestWithBasicAnswerCallback(socket, token, quest, t, timeout))
				return true;
			else
			{
				delete t;
				return false;
			}
		}

	};
}

#endif

