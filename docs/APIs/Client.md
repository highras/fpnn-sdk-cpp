## Client

### 介绍

[TCPClient](TCPClient.md) 和 [UDPClient](UDPClient.md) 的基类。提供客户端的通用方法。

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 关键定义

	class Client
	{
	public:
		static const char* SDKVersion;

	public:
		Client(const std::string& host, int port, bool autoReconnect = true);
		virtual ~Client();

		inline bool connected();
		inline const std::string& endpoint();
		inline int socket();
		inline ConnectionInfoPtr connectionInfo();

		inline void setQuestProcessor(IQuestProcessorPtr processor);

		inline void setQuestTimeout(int64_t seconds);
		inline int64_t getQuestTimeout();

		inline bool isAutoReconnect();
		inline void setAutoReconnect(bool autoReconnect);

		virtual bool connect() = 0;
		virtual bool asyncConnect() = 0;
		virtual void close();				//-- Please MUST implement this 'close()' function for specific implementations.
		virtual bool reconnect();
		virtual bool asyncReconnect();
		virtual void keepAlive() = 0;

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.

			timeout in seconds.
		*/
		virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0) = 0;
		virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0) = 0;
		virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0) = 0;

		static TCPClientPtr createTCPClient(const std::string& host, int port, bool autoReconnect = true);
		static TCPClientPtr createTCPClient(const std::string& endpoint, bool autoReconnect = true);

		static UDPClientPtr createUDPClient(const std::string& host, int port, bool autoReconnect = true);
		static UDPClientPtr createUDPClient(const std::string& endpoint, bool autoReconnect = true);

		/*===============================================================================
		  Interfaces for embed mode.
		=============================================================================== */
		/*
		* 链接建立和关闭事件，用 IQuestProcessor 封装。
		* server push / duplex，采用 EmbedRecvNotifyDelegate 通知接口，由上层语言/封装处理。
		*/
		virtual void embed_configRecvNotifyDelegate(EmbedRecvNotifyDelegate delegate) { _embedRecvNotifyDeleagte = delegate; }

		/*
		* 如果返回成功，rawData 由 C++ SDK 负责释放(delete), 如果失败，需要调用者释放。
		*/
		virtual bool embed_sendData(std::string* rawData) = 0;
	};

	typedef std::shared_ptr<Client> ClientPtr;

### 构造函数

	Client(const std::string& host, int port, bool autoReconnect = true);

**参数说明**

* **`const std::string& host`**

	服务器 IP 地址或者域名。

* **`int port`**

	服务器端口。

* **`bool autoReconnect`**

	启用或者禁用自动重连功能。

### 公有域

#### SDKVersion

	static const char* SDKVersion;

当前 SDK 版本号。

### 成员函数

#### connected

	inline bool connected();

判断链接是否已经建立。

#### endpoint

	inline const std::string& endpoint();

返回要链接的对端服务器的 endpoint。

#### socket

	inline int socket();

返回当前 socket。


#### connectionInfo

	inline ConnectionInfoPtr connectionInfo();

返回当前的连接信息对象。

**注意**

每次链接，连接信息对象均不相同。


#### setQuestProcessor

	inline void setQuestProcessor(IQuestProcessorPtr processor);

配置链接事件和 Server Push 请求处理模块。具体可参见 [IQuestProcessor](IQuestProcessor.md)。

#### setQuestTimeout

	inline void setQuestTimeout(int64_t seconds);

设置当前 Client 实例的请求超时。单位：秒。`0` 表示使用 ClientEngine 的请求超时设置。

#### getQuestTimeout

	inline int64_t getQuestTimeout();

获取当前 Client 实例的请求超时设置。

#### isAutoReconnect

	inline bool isAutoReconnect();

判断是否配置为自动重连。

#### setAutoReconnect

	inline void setAutoReconnect(bool autoReconnect);

修改自动重连设置。

#### connect

	virtual bool connect() = 0;

同步连接服务器。

#### asyncConnect

	virtual bool asyncConnect() = 0;

异步连接服务器。

#### close

	virtual void close();

关闭当前连接。

#### reconnect

	virtual bool reconnect();

同步重连。

#### asyncReconnect

	virtual bool asyncReconnect();

异步重连。

#### keepAlive

	virtual void keepAlive() = 0;

开启连接自动保活/保持连接。

#### sendQuest

	virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0) = 0;
	virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0) = 0;
	virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0) = 0;

发送请求。

第一个声明为同步发送，后两个声明为异步发送。

**参数说明**

* **`FPQuestPtr quest`**

	请求数据对象。具体请参见 [FPQuest](FPQuest.md)。

	**注意**

	对于 UDP 连接，oneway 消息被视为可丢弃消息；twoway 消息被视为不可丢弃消息。如果需要显式指定消息的可丢弃属性，请使用 [UDPClient](UDPClient.md) 的 [sendQuestEx](UDPClient.md#sendQuestEx) 接口。

* **`int timeout`**

	本次请求的超时设置。单位：秒。

	**注意**

	如果 `timeout` 为 0，表示使用 Client 实例当前的请求超时设置。  
	如果 Client 实例当前的请求超时设置为 0，则使用 ClientEngine 的请求超时设置。

* **`AnswerCallback* callback`**

	异步请求的回调对象。具体请参见 [AnswerCallback](AnswerCallback.md)。

* **`std::function<void (FPAnswerPtr answer, int errorCode)> task`**

	异步请求的回调函数。

	**注意**

	+ 如果遇到连接中断/结束，连接已关闭，超时等情况，`answer` 将为 `nullptr`。
	+ 当且仅当 `errorCode == FPNN_EC_OK` 时，`answer` 为业务正常应答；否则其它情况，如果 `answer` 不为 `nullptr`，则为 FPNN 异常应答。

		FPNN 异常应答请参考 [errorAnswer](FPWriter.md#errorAnswer)。

**返回值说明**

* **FPAnswerPtr**

	+ 对于 oneway 请求，FPAnswerPtr 的返回值**恒定为** `nullptr`。
	+ 对于 twoway 请求，FPAnswerPtr 可能为正常应答，也可能为异常应答。FPNN 异常应答请参考 [errorAnswer](FPWriter.md#errorAnswer)。

* **bool**

	发送成功，返回 true；失败 返回 false。

	**注意**

	如果发送成功，`AnswerCallback* callback` 将不能再被复用，用户将无须处理 `callback` 对象的释放。SDK 会在合适的时候，调用 `delete` 操作进行释放；  
	如果返回失败，用户需要处理 `AnswerCallback* callback` 对象的释放。

#### createTCPClient

	static TCPClientPtr createTCPClient(const std::string& host, int port, bool autoReconnect = true);
	static TCPClientPtr createTCPClient(const std::string& endpoint, bool autoReconnect = true);

创建 TCP 客户端。

**参数说明**

* **`const std::string& host`**

	服务器 IP 或者域名。

* **`int port`**

	服务器端口。

* **`const std::string& endpoint`**

	服务器 endpoint。

* **`bool autoReconnect`**

	是否自动重连。

#### createUDPClient

	static UDPClientPtr createUDPClient(const std::string& host, int port, bool autoReconnect = true);
	static UDPClientPtr createUDPClient(const std::string& endpoint, bool autoReconnect = true);

创建 UDP 客户端。

**参数说明**

* **`const std::string& host`**

	服务器 IP 或者域名。

* **`int port`**

	服务器端口。

* **`const std::string& endpoint`**

	服务器 endpoint。

* **`bool autoReconnect`**

	是否自动重连。

#### embed_configRecvNotifyDelegate

	virtual void embed_configRecvNotifyDelegate(EmbedRecvNotifyDelegate delegate);

**注意：嵌入模式专用**

用于 Java、Unity、Go、C# 等语言封装 C++ SDK 时，C++ SDK 将收到的 FPNN 协议数据不经解包解码，直接转发给上层语言进行处理。

具体请参见 [嵌入模式](../embedMode.md) 及 [embedTypes.h](../../src/core/embedTypes.h) 内注释。

#### embed_sendData

	virtual bool embed_sendData(std::string* rawData) = 0;

**注意：嵌入模式专用**

用于 Java、Unity、Go、C# 等语言封装 C++ SDK 时，上层语言通过 C++ SDK 直接发送 FPNN 协议数据，C++ SDK 层面不再做额外处理。

具体请参见 [嵌入模式](../embedMode.md)。

**参数说明**

* **`std::string* rawData`**

	需要发送的，已经编码完毕，封装完毕的 FPNN 协议数据。

**返回值说明**

返回 true 表示成功，返回 false 表示失败。

**注意**

如果返回 true，`std::string* rawData` 将由 SDK 在合适的时候调用 `delete` 进行释放。  
如果返回 false，使用者需要负责 `std::string* rawData` 对象的释放。

