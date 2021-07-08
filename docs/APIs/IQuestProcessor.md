## IQuestProcessor

### 介绍

IQuestProcessor 为链接事件和 Server Push 请求的处理模块的基类。

无论是处理链接事件，还是 Server Push 请求，均须从 IQuestProcessor 派生实际的处理子类，然后通过 Client/TCPClient/UDPClient 的 [setQuestProcessor](Client.md#setQuestProcessor) 接口进行设置。

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 关键定义

	class IQuestProcessor
	{
	public:
		IQuestProcessor();
		virtual ~IQuestProcessor();

	protected:
		bool sendAnswer(FPAnswerPtr answer);
		IAsyncAnswerPtr genAsyncAnswer();

	public:
		inline QuestSenderPtr genQuestSender(const ConnectionInfo& connectionInfo);

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
		virtual FPAnswerPtr unknownMethod(const std::string& method_name, const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& connInfo);
	};

	#define QuestProcessorClassPrivateFields(processor_name)

	#define QuestProcessorClassBasicPublicFuncs

### 构造函数

	IQuestProcessor();

### 成员函数

#### sendAnswer

	bool sendAnswer(FPAnswerPtr answer);

在 Server Push 的请求处理函数内，为 twoway 类型的请求，提前发送应答。

**注意**

+ 调用该函数后，Server Push 的请求处理函数必须返回 `nullptr`。
+ 该函数必须在 Server Push 的请求处理函数内(或请求处理函数返回前，同一线程上，被请求处理函数调用的其他函数中)被调用，否则无效。
+ 在 Server Push 的请求处理函数内，该函数只能调用一次，后续调用无效。

**返回值说明**

如果发送成功，返回 true；如果发送失败，或者是 oneway 类型消息，或者前面已经调用过该成员函数，则返回 false。

#### genAsyncAnswer

	IAsyncAnswerPtr genAsyncAnswer();

在 Server Push 的请求处理函数内，为当前twoway 类型的请求生成异步返回器。

异步返回器请参见 [IAsyncAnswer](IAsyncAnswer.md)。

**注意**

+ 调用该函数后，Server Push 的请求处理函数必须返回 `nullptr`。
+ 该函数必须在 Server Push 的请求处理函数内(或请求处理函数返回前，同一线程上，被请求处理函数调用的其他函数中)被调用，否则无效。
+ 在 Server Push 的请求处理函数内，该函数只能调用一次，后续调用无效。

#### genQuestSender

	inline QuestSenderPtr genQuestSender(const ConnectionInfo& connectionInfo);

使用连接信息对象的**原本**实例，生成请求发送对象。

请求发送对象请参见 [QuestSender](QuestSender.md)。

**注意**

使用连接信息对象的**副本**实例无效。

#### sendQuest

	virtual FPAnswerPtr sendQuest(FPQuestPtr quest, int timeout = 0);
	virtual bool sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout = 0);
	virtual bool sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout = 0);

在 Server Push 的请求处理函数内，向服务端再次发送请求的便捷接口。

**注意**

该函数必须在 Server Push 的请求处理函数内(或请求处理函数返回前，同一线程上，被请求处理函数调用的其他函数中)被调用，否则无效。

**参数说明**

* **`FPQuestPtr quest`**

	请求数据对象。具体请参见 [FPQuest](FPQuest.md)。

	**注意**

	对于 UDP 连接，oneway 消息被视为可丢弃消息；twoway 消息被视为不可丢弃消息。如果需要显式指定消息的可丢弃属性，请使用 [sendQuestEx](#sendQuestEx) 接口。

* **`int timeout`**

	本次请求的超时设置。单位：秒。

	**注意**

	如果 `timeout` 为 0，表示使用 ClientEngine 的请求超时设置。

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

#### sendQuestEx

	virtual FPAnswerPtr sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec = 0);
	virtual bool sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec = 0);
	virtual bool sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec = 0);

在 Server Push 的请求处理函数内，向服务端再次发送请求的便捷接口扩展版。

**注意**

该函数必须在 Server Push 的请求处理函数内(或请求处理函数返回前，同一线程上，被请求处理函数调用的其他函数中)被调用，否则无效。

**参数说明**

* **`FPQuestPtr quest`**

	请求数据对象。具体请参见 [FPQuest](FPQuest.md)。

* **`bool discardable`**

	对于 UDP 连接，数据包是否可丢弃/是否是必达消息。TCP 连接忽略该参数。

* **`int timeoutMsec`**

	本次请求的超时设置。单位：**毫秒**。

	**注意**

	如果 `timeoutMsec` 为 0，表示使用 ClientEngine 的请求超时设置。

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

#### connected

	virtual void connected(const ConnectionInfo&) = delete;
	virtual void connected(const ConnectionInfo&, bool connected);

链接建立事件。

**注意**

链接建立事件函数返回后，该连接上数据才可以正常发送。

**参数说明**

* **`const ConnectionInfo&`**

	连接信息对象（**实例原本**）。

* **`bool connected`**

	连接是否成功建立。

#### connectionWillClose

	virtual void connectionWillClose(const ConnectionInfo& connInfo, bool closeByError);

链接即将关闭事件。

**注意**

链接即将关闭事件函数返回后，socket 进入失效，或者关闭状态。但因为**操作系统线程调度**的原因，链接即将关闭事件被触发时，可能会有同一 socket 上的 Server Push 请求正在处理。

**参数说明**

* **`const ConnectionInfo& connInfo`**

	连接信息对象（**实例原本**）。

* **`bool closeByError`**

	连接正常关闭，还是(因错误)异常关闭。

#### unknownMethod

	virtual FPAnswerPtr unknownMethod(const std::string& method_name, const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& connInfo);

非法请求处理函数。

**参数说明**

* **`const std::string& method_name`**

	非法请求请求的接口名/方法名。

* **`const FPReaderPtr args`**

	非法请求所含数据的读取器。参见 [FPQReader](FPReader.md#FPQReader)。

* **`const FPQuestPtr quest`**

	非法请求所对应的 [FPQuest](FPQuest.md) 对象。

* **`const ConnectionInfo& connInfo`**

	连接信息对象（**实例原本**）。

#### registerMethod

	inline void registerMethod(const std::string& method_name, MethodFunc func);

注册 Server Push 请求处理函数。

**参数说明**

* **`const std::string& method_name`**

	注册的接口名称/方法名称。

* **`MethodFunc func`**

	注册的处理函数。

	**注意**

	+ 处理函数必须是**非静态**的成员函数。
	+ 处理函数签名参见 [Server请求处理函数](#Server请求处理函数)。

#### Server请求处理函数

	FPAnswerPtr MethodFunc(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);

Server Push 请求处理函数签名原型。

**参数说明**

* **`const FPReaderPtr`**

	Server Push 请求所含数据的读取器。参见 [FPQReader](FPReader.md#FPQReader)。

* **`const FPQuestPtr`**

	Server Push 请求所对应的 [FPQuest](FPQuest.md) 对象。

* **`const ConnectionInfo&`**

	连接信息对象（**实例原本**）。

### 宏定义

#### QuestProcessorClassPrivateFields

	#define QuestProcessorClassPrivateFields(processor_name)

在每一个 IQuestProcessor 的子类中，必须在**私有域**里声明的宏。

负责给 IQuestProcessor 的子类添加基类接口需要的成员对象。

**参数说明**

* **`processor_name`**

	IQuestProcessor 子类类名。

#### QuestProcessorClassBasicPublicFuncs

	#define QuestProcessorClassBasicPublicFuncs

在每一个 IQuestProcessor 的子类中，必须在**公有域**里声明的宏。

负责给 IQuestProcessor 的子类添加工具接口，和非文档化的基类虚方法的实现。