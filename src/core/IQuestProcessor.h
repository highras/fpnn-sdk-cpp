#ifndef FPNN_Quest_Processor_Interface_H
#define FPNN_Quest_Processor_Interface_H

#include <memory>
#include <sstream>
#include <unordered_map>
#include <netinet/in.h>
#include "FPReader.h"
#include "FPWriter.h"
#include "NetworkUtility.h"
#include "AnswerCallbacks.h"
#include "ConcurrentSenderInterface.h"

namespace fpnn
{
	class IQuestProcessor;
	class ConnectionInfo;
	class QuestSender;
	class IAsyncAnswer;
	typedef std::shared_ptr<IQuestProcessor> IQuestProcessorPtr;
	typedef std::shared_ptr<ConnectionInfo> ConnectionInfoPtr;
	typedef std::shared_ptr<QuestSender> QuestSenderPtr;
	typedef std::shared_ptr<IAsyncAnswer> IAsyncAnswerPtr;

	//=================================================================//
	//- Connection Info:
	//-     Just basic info to identify a connection.
	//=================================================================//
	class ConnectionInfo
	{
	private:
		static std::atomic<uint64_t> uniqueIdBase;
	private:
		friend class Client;
		friend class TCPClient;
		friend class UDPClient;
		friend class TCPClientConnection;
		friend class UDPClientConnection;
		friend class IQuestProcessor;

		std::mutex* _mutex;		//-- only for sync quest to set answer map.
		uint64_t _uniqueId;
		bool _isTCP;
		bool _isIPv4;
		bool _encrypted;
		bool _internalAddress;

		//-- Only use for UDP.
		uint8_t* _socketAddress;

		ConnectionInfo(int socket_, int port_, const std::string& ip_, bool isIPv4): _mutex(0),
			_isTCP(true), _isIPv4(isIPv4), _encrypted(false), _internalAddress(false), _socketAddress(NULL),
			token(0), socket(socket_), port(port_), ip(ip_)
		{
			if (isIPv4)
			{
				_internalAddress = NetworkUtil::isPrivateIPv4(ip);
			}
			else
				_internalAddress = NetworkUtil::isPrivateIPv6(ip);

			_uniqueId = ++uniqueIdBase;
		}

		void changeToUDP(int newSocket, uint8_t* socketAddress)	//-- UDP Client Used.
		{
			_isTCP = false;
			socket = newSocket;
			
			if (_socketAddress)
				free(_socketAddress);

			_socketAddress = socketAddress;
		}

		void changeToUDP()
		{
			_isTCP = false;
		}

	public:
		uint64_t token;
		int socket;
		int port;
		std::string ip;

		std::string str() const {
			std::stringstream ss;
			ss << "Socket: " << socket << ", address: " << ip << ":" <<port;
			return ss.str();
		}
		
		std::string endpoint() const {
			return ip + ":" + std::to_string(port);
		}

		inline bool isTCP() const { return _isTCP; }
		inline bool isUDP() const { return !_isTCP; }
		inline bool isEncrypted() const { return _encrypted; }
		inline bool isPrivateIP() const { return _internalAddress; }
		inline uint64_t uniqueId() const { return _uniqueId; }

		ConnectionInfo(const ConnectionInfo& ci): _mutex(0), _uniqueId(ci._uniqueId),
			_isTCP(ci._isTCP), _isIPv4(ci._isIPv4), _encrypted(ci._encrypted),
			_internalAddress(ci._internalAddress), _socketAddress(NULL),
			token(ci.token), socket(ci.socket), port(ci.port), ip(ci.ip)
		{
			if (ci._socketAddress)
			{
				int len = ci._isIPv4 ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
				_socketAddress = (uint8_t*)malloc(len);
				memcpy(_socketAddress, ci._socketAddress, len);
			}
		}

		~ConnectionInfo()
		{
			if (_socketAddress)
				free(_socketAddress);
		}
	};

	//=================================================================//
	//- Quest Sender:
	//-     Just using for send quest to peer.
	//-		For all sendQuest() interfaces:
	//-     	If throw exception or return false, caller must free quest & callback.
	//-			If return true, or FPAnswerPtr is NULL, don't free quest & callback.
	//=================================================================//
	class QuestSender
	{
		int _socket;
		uint64_t _token;
		std::mutex* _mutex;
		IConcurrentSender* _concurrentSender;

	public:
		QuestSender(IConcurrentSender* concurrentSender, const ConnectionInfo& ci, std::mutex* mutex):
			_socket(ci.socket), _token(ci.token), _mutex(mutex), _concurrentSender(concurrentSender) {}

		virtual ~QuestSender() {}
		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.
		*/
		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0)
		{
			return _concurrentSender->sendQuest(_socket, _token, _mutex, quest, timeout * 1000);
		}
	
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0)
		{
			return _concurrentSender->sendQuest(_socket, _token, quest, callback, timeout * 1000);
		}
	
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0)
		{
			return _concurrentSender->sendQuest(_socket, _token, quest, std::move(task), timeout * 1000);
		}

		virtual FPAnswerPtr sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec = 0);
	};

	//=================================================================//
	//- Async Answer Interface:
	//-     Just using for delay send answer after the noraml flow.
	//-     Before then end of normal flow, gen the instance of Async Answer Interface,
	//-     then, return NULL or nullptr as the return value for normal flow in IQuestProcessor.
	//=================================================================//
	class IAsyncAnswer
	{
	public:
		virtual ~IAsyncAnswer() {}

		virtual const FPQuestPtr getQuest() = 0;
		/** need release answer anyway. */
		virtual bool sendAnswer(FPAnswerPtr) = 0;
		virtual bool isSent() = 0;

		//-- Extends
		virtual void cacheTraceInfo(const std::string&) = 0;
		virtual void cacheTraceInfo(const char *) = 0;

		virtual bool sendErrorAnswer(int code = 0, const std::string& ex = "")
		{
			FPAnswerPtr answer = FPAWriter::errorAnswer(getQuest(), code, ex);
			return sendAnswer(answer);
		}
		virtual bool sendErrorAnswer(int code = 0, const char* ex = "")
		{
			FPAnswerPtr answer = FPAWriter::errorAnswer(getQuest(), code, ex);
			return sendAnswer(answer);
		}
		virtual bool sendEmptyAnswer()
		{
			FPAnswerPtr answer = FPAWriter::emptyAnswer(getQuest());
			return sendAnswer(answer);
		}
	};

	//=================================================================//
	//- Quest Processor Interface (Quest Handler Interface):
	//-     "Call by framwwork." section are used internal by framework.
	//-     "Call by Developer." section are using by developer.
	//=================================================================//
	class IQuestProcessor
	{
	private:
		IConcurrentSender* _concurrentSender;

	public:
		IQuestProcessor(): _concurrentSender(0) {}
		virtual ~IQuestProcessor() {}

		/*===============================================================================
		  Call by framwwork.
		=============================================================================== */
		/** Call by framework. */
		void initAnswerStatus(ConnectionInfoPtr connInfo, FPQuestPtr quest);
		bool getQuestAnsweredStatus();
		bool finishAnswerStatus();	//-- return answer status

		virtual void setConcurrentSender(IConcurrentSender* concurrentSender) { if (!_concurrentSender) _concurrentSender = concurrentSender; }
		virtual FPAnswerPtr processQuest(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& connectionInfo) { return nullptr; }

		/*===============================================================================
		  Call by Developer.
		=============================================================================== */
	protected:
		/*
			The following four methods ONLY can be called in server interfaces function which is called by FPNN framework.
		*/
		bool sendAnswer(FPAnswerPtr answer);
		inline bool sendAnswer(const FPQuestPtr, FPAnswerPtr answer)	//-- for compatible old version.
		{
			return sendAnswer(answer);
		}
		IAsyncAnswerPtr genAsyncAnswer();
		inline IAsyncAnswerPtr genAsyncAnswer(const FPQuestPtr)	//-- for compatible old version.
		{
			return genAsyncAnswer();
		}

	public:
		inline QuestSenderPtr genQuestSender(const ConnectionInfo& connectionInfo)
		{
			return std::make_shared<QuestSender>(_concurrentSender, connectionInfo, connectionInfo._mutex);
		}

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.
		*/
		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

		virtual FPAnswerPtr sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec = 0);

		/*===============================================================================
		  Event Hook. (Common)
		=============================================================================== */
		virtual void connected(const ConnectionInfo&) = delete;
		virtual void connected(const ConnectionInfo&, bool connected) {}
		virtual void connectionWillClose(const ConnectionInfo& connInfo, bool closeByError) {}
		virtual FPAnswerPtr unknownMethod(const std::string& method_name, const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& connInfo) 
		{
			if (quest->isTwoWay()){
				return FPAWriter::errorAnswer(quest, FPNN_EC_CORE_UNKNOWN_METHOD, "Unknow method:" + method_name, connInfo.str().c_str());
			}
			else{
				LOG_ERROR("OneWay Quest, UNKNOWN method:%s. %s", method_name.c_str(), connInfo.str().c_str());
				return NULL;
			}
		}
	};

	#define QuestProcessorClassPrivateFields(processor_name)	\
	public:	\
		typedef FPAnswerPtr (processor_name::* MethodFunc)(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);\
	private:\
		std::unordered_map<std::string, MethodFunc> _methodMap;\

	#define QuestProcessorClassBasicPublicFuncs	\
		inline void registerMethod(const std::string& method_name, MethodFunc func)	{ \
			_methodMap[method_name] = func; \
		} \
		virtual FPAnswerPtr processQuest(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& connInfo)	\
		{	\
			const std::string& method = quest->method();	\
			auto iter = _methodMap.find(method);	\
			if (iter != _methodMap.end()) {	\
				return (this->*(iter->second))(args, quest, connInfo); }	\
			else  \
				return unknownMethod(method, args, quest, connInfo);	\
		}
}

#endif
