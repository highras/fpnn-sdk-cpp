#ifndef FPNN_IO_Worker_H
#define FPNN_IO_Worker_H

#include <time.h>
#include <unistd.h>
#include "IQuestProcessor.h"
#include "embedTypes.h"

namespace fpnn
{
	//===============[ IReleaseable ]=====================//
	class IReleaseable
	{
	public:
		virtual bool releaseable() = 0;
		virtual ~IReleaseable() {}
	};
	typedef std::shared_ptr<IReleaseable> IReleaseablePtr;

	//===============[ Connection Status ]=====================//
	/*
	*	设计思想：
	*		1. 任何用不到的指令都需要耗费CPU资源；
	*		2. 不做任何用不到的处理，即使这将导致逻辑不完备，或者代码难以理解。
	*
	*	因此：
	*		本结构的逻辑处理并不完备，但足够 C++ SDK 流程使用，并保证流程正确。
	*/
	struct ConnectionEventStatus
	{
		//-- Status:
		//--	0: undo;
		//--	1: doing;
		//--	2: done.
		
		unsigned int _connectedEventStatus:2;
		unsigned int _closeEventStatus:2;
		unsigned int _connectionDiscarded:1;
		unsigned int _requireCallCloseEvent:1;

		ConnectionEventStatus(): _connectedEventStatus(0), _closeEventStatus(0),
			_connectionDiscarded(0), _requireCallCloseEvent(0) {}

		inline void connectionDiscarded() { _connectionDiscarded = 1; }
		inline bool getConnectedEventCallingPermission(bool& requireCallConnectionCannelledEvent)
		{
			requireCallConnectionCannelledEvent = false;
			if (_connectedEventStatus == 0)
			{
				if (_connectionDiscarded == 1)
					requireCallConnectionCannelledEvent = true;

				_connectedEventStatus = 1;
				return true;
			}

			return false;
		}
		inline void connectedEventCalled(bool& requireCallCloseEvent)
		{
			_connectedEventStatus = 2;
			requireCallCloseEvent = this->_requireCallCloseEvent == 1;
		}
		inline bool getCloseEventCallingPermission(bool& requireCallConnectionCannelledEvent)
		{
			requireCallConnectionCannelledEvent = false;

			if (_closeEventStatus != 0)
				return false;

			if (_connectedEventStatus == 1)
			{
				_requireCallCloseEvent = 1;
				return false;
			}
			if (_connectedEventStatus == 0)
			{
				_closeEventStatus = 1;
				_connectionDiscarded = 1;
				_connectedEventStatus = 1;
				requireCallConnectionCannelledEvent = true;
				return false;
			}
			if (_connectedEventStatus == 2)
			{
				_closeEventStatus = 1;
				return true;
			}
			return true;
		}
	};

	//===============[ Basic Connection ]=====================//
	class BasicConnection: public IReleaseable
	{
	public:
		enum ConnectionType
		{
			TCPClientConnectionType,
			UDPClientConnectionType
		};

	protected:
		std::mutex _mutex;
		IQuestProcessorPtr _questProcessor;
		ConnectionEventStatus _connectionEventStatus;

	public:
		ConnectionInfoPtr _connectionInfo;

		int64_t _activeTime;
		std::atomic<int> _refCount;

		std::unordered_map<uint32_t, BasicAnswerCallback*> _callbackMap;

	public:
		BasicConnection(ConnectionInfoPtr connectionInfo): _connectionInfo(connectionInfo), _refCount(0)
		{
			_connectionInfo->token = (uint64_t)this;	//-- if use Virtual Derive, must redo this in subclass constructor.
			_activeTime = time(NULL);
		}

		virtual ~BasicConnection()
		{
			close(_connectionInfo->socket);
		}

		virtual bool waitForSendEvent() = 0;
		virtual enum ConnectionType connectionType() = 0;
		virtual bool releaseable() { return (_refCount == 0); }

		inline int socket() const { return _connectionInfo->socket; }
		inline IQuestProcessorPtr questProcessor() { return _questProcessor; }
		virtual void embed_configRecvNotifyDelegate(EmbedRecvNotifyDelegate delegate) = 0;

		//--------------- Connection event status ------------------//
		inline void connectionDiscarded()
		{
			std::unique_lock<std::mutex> lck(_mutex);
			_connectionEventStatus.connectionDiscarded();
		}
		inline bool getConnectedEventCallingPermission(bool& requireCallConnectionCannelledEvent)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			return _connectionEventStatus.getConnectedEventCallingPermission(requireCallConnectionCannelledEvent);
		}
		inline void connectedEventCalled(bool& requireCallCloseEvent)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			_connectionEventStatus.connectedEventCalled(requireCallCloseEvent);
		}
		inline bool getCloseEventCallingPermission(bool& requireCallConnectionCannelledEvent)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			return _connectionEventStatus.getCloseEventCallingPermission(requireCallConnectionCannelledEvent);
		}
	};
}

#endif