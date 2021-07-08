## Guide

### 基本使用向导

直观的案例：

	using namespace fpnn;

	TCPClientPtr client = TCPClient::createClient("example.com:12345");

	FPQWriter qw("two way demo", 2);
	qw.param("param 1", "one");
	qw.param("param 2", 2); 

	FPQuestPtr quest = qw.take();
	FPAnswerPtr answer = client->sendQuest(quest);
	FPAReader ar(answer);

	std::cout<<"Answer item 1: "<<ar.wantInt("int item");
	std::cout<<"Answer item 2: "<<ar.wantString("string item");


1. **术语**

	* **双向请求/two way**

		请求的发送方需要处理方回应 answer 的 quest 类型。  
		一个 Answer 对应一个 Quest。

	* **单向请求/one way**

		请求的发送方**不需要**处理方回应 answer 的 quest 类型。  
		Quest 没有对应的 Answer。

	* **duplex**

		双工。与支持 server push 同含义。

	* **server push**

		服务端向客户端发送请求。请求可以是双向请求/twoway，也可以是单向请求/oneway。

1. **头文件**

	* 如果使用`release.sh`生成的`release`目录里的静态库和头文件，仅需包含头文件`fpnn.h`：

			#include "fpnn.h"

	* 如果直接使用 SDK 源代码目录和`make`生成的库文件，则按需包含头文件`core/TCPClient.h`和`core/UDPClient.h`：

			#include "core/TCPClient.h"		//-- 按需包含
			#include "core/UDPClient.h"		//-- 按需包含

1. **命名空间**

	SDK 所有对象，均处于`fpnn`命名空间中。

		using namespace fpnn;

1. **创建客户端对象实例**

	TCP 及 UDP 客户端。可用以下方法创建：

		//-- TCP Client
		TCPClientPtr client = Client::createTCPClient(const std::string& host, int port, bool autoReconnect = true);
		TCPClientPtr client = Client::createTCPClient(const std::string& endpoint, bool autoReconnect = true);
		TCPClientPtr client = TCPClient::createClient(const std::string& host, int port, bool autoReconnect = true);
		TCPClientPtr client = TCPClient::createClient(const std::string& endpoint, bool autoReconnect = true);

		//-- UDP Client
		UDPClientPtr client = Client::createUDPClient(const std::string& host, int port, bool autoReconnect = true);
		UDPClientPtr client = Client::createUDPClient(const std::string& endpoint, bool autoReconnect = true);
		UDPClientPtr client = UDPClient::createClient(const std::string& host, int port, bool autoReconnect = true);
		UDPClientPtr client = UDPClient::createClient(const std::string& endpoint, bool autoReconnect = true);

	其中 Client 为 TCPClient 和 UDPClient 的父类对象。  
	ClientPtr、TCPClientPtr、UDPClientPtr 为对应的 shared_ptr。  
	ClientPtr 可指向 TCPClientPtr 或 UDPClientPtr 对象实例。

1. **[可选] 配置客户端**

	* TCP 客户端

		TCP 客户端可配置加密链接，链接自动保活(显式)，全局所有连接强制保活(隐式)，和连接超时时间、请求超时时间。

		具体请参见：

		+ [Client](APIs/Client.md)
		+ [TCPClient](APIs/TCPClient.md)
		+ [Config](APIs/Config.md)

	* UDP 客户端

		UDP 客户端可配置链接自动保活，MTU，链接丢失时间。

		具体请参见：

		+ [Client](APIs/Client.md)
		+ [UDPClient](APIs/UDPClient.md)
		+ [Config](APIs/Config.md)
	

1. **[可选] 连接服务器**

	**如果创建客户端实例时，`bool autoReconnect` 参数设置为 `true`，则在发送请求时，会自动连接，而无需显示调用连接接口。**

	* **同步连接**

			bool client->connect();

		同步连接会阻塞当前线程，直到链接建立完成。

	* **异步连接**

			bool client->asyncConnect();

		异步连接，不会阻塞当前线程（为 UDPClient 设置了链接建立事件时除外，具体请参见 [UDPClient](APIs/UDPClient.md#asyncConnect)）。


1. **发送请求**


	1. **生成请求对象**

			FPQWriter qw("two way demo", 3);
			qw.param("string", "one");
    		qw.param("int", 2); 
    		qw.param("double", 3.3);
    		FPQuestPtr quest = qw.take();

    	FPQWriter 请参见 [FPQWriter](APIs/FPWriter.md#FPQWriter); FPQuest/FPQuestPtr 请参见：[FPQuest](APIs/FPQuest.md)。

	1. **发送请求**

			//-- 同步发送
			FPAnswerPtr answer = client->sendQuest(quest);

			//-- 异步发送
			bool success = client->sendQuest(quest, [](FPAnswerPtr answer, int errorCode){
				if (errorCode == FPNN_EC_OK)
				{
					//-- process business answer ...
				}
				else
				{
					if (answer)
						//-- process FPNN standard error
					else
						//-- answer is nullptr, only errorCode is useful.
				}
			});

		sendQuest 请参见[sendQuest](APIs/Client.md#sendQuest)。

		对 UDP 的扩展 sendQuestEx 请参见[sendQuestEx](APIs/UDPClient.md#sendQuestEx)。


	1. **获取返回数据**

			FPAReader ar(answer);

			std::cout<<"Answer item 1: "<<ar.wantInt("int item");
			std::cout<<"Answer item 2: "<<ar.wantString("string item");

		FPAnswer/FPAnswerPtr 请参见 [FPAnswer](APIs/FPAnswer.md)。


1. **[可选] 断开连接**

		void client->close();

	客户端析构时，会自动关闭。

### 链接事件

1. 连接建立事件

	原型：

		virtual void connected(const ConnectionInfo&, bool connected);

	连接建立事件完成前，对应连接上的数据将无法发送和接收。

	具体请参见 [连接建立事件](APIs/IQuestProcessor.md#connected)。


1. 连接即将断开事件

	原型：

		virtual void connectionWillClose(const ConnectionInfo& connInfo, bool closeByError);

	链接即将关闭事件完成后，socket 进入失效或关闭状态。

	具体请参见 [连接即将关闭事件](APIs/IQuestProcessor.md#connectionWillClose)。


### 处理服务器推送(Server Push)

服务器推送(Server Push/Duplex)为服务器向客户端发送请求，客户端处理完后发送响应。

一般而言，对于每个服务器请求，客户端需要注册一个处理函数，和服务器所请求的接口/方法所绑定。

服务器推送请求处理函数原型：

	FPAnswerPtr MethodFunc(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);

具体可参见 [Server请求处理函数](APIs/IQuestProcessor.md#Server请求处理函数)。

服务器推送请求处理函数注册函数：

	inline void IQuestProcessor::registerMethod(const std::string& method_name, MethodFunc func);

具体可参见 [registerMethod](APIs/IQuestProcessor.md#registerMethod)。

1. **服务器请求处理函数的返回**

	对于 oneway 的请求/消息，客户端无需回应服务器。处理函数直接返回 nullptr 即可。

	对于 twoway 的请求/消息，客户端的请求处理函数必须生成有效的 FPAnswerPtr 并返回。

	* **twoway 请求的两种非常规应答方式**

		+ **提前返回**

			当返回数据已经准备完成，但处理函数还有事情需要继续处理时，可以使用以下接口提前返回：

				bool IQuestProcessor::sendAnswer(FPAnswerPtr answer);

			具体可参见 [sendAnswer](APIs/IQuestProcessor.md#sendAnswer)。

			一旦调用 sendAnswer 之后，客户端的请求处理函数必须返回 nullptr。

			**注意**

			提前返回等价于执行没有线程切换的异步任务(提前返回后，请求处理函数继续执行的任务)。

		+ **异步返回**

			当返回结果并未准备好，需要在其他地方，甚至其他线程返回时，可以使用异步返回。

			1. 生成异步返回器：

					IAsyncAnswerPtr IQuestProcessor::genAsyncAnswer();

				具体可参见 [genAsyncAnswer](APIs/IQuestProcessor.md#genAsyncAnswer)。

			1. 调用异步返回器的返回接口，发送应答对象：

					IAsyncAnswerPtr async = genAsyncAnswer();
					async->sendAnswer(answer);

				具体可参见 [sendAnswer](APIs/IAsyncAnswer.md#sendAnswer)。

			一旦生成异步返回器后，请求处理函数必须返回 nullptr。

