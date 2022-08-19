## UDPClient

### 介绍

UDP 客户端。[Client](Client.md) 的子类。

UDP 客户端内部实现了可靠 UDP 连接，可混合发送可靠和不可靠数据。

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 关键定义

	class UDPClient: public Client, public std::enable_shared_from_this<UDPClient>
	{
	public:
		virtual ~UDPClient() {}

		/*
		*	因为 UDP Client 的异步连接操作，只有**连接建立事件**需要异步。但一般情况下，几乎不会用到该事件。
		*	因此为了简便起见，asyncConnect() 直接调用 connect()。
		*	除非有特殊需求，再做成 TCP Client 那样复杂的纯异步操作。
		*/
		virtual bool asyncConnect() { return connect(); }
		
		inline static UDPClientPtr createClient(const std::string& host, int port, bool autoReconnect = true);
		inline static UDPClientPtr createClient(const std::string& endpoint, bool autoReconnect = true);

		bool enableEncryptorByDerData(const std::string &derData, bool packageReinforce = false, bool dataEnhance = false, bool dataReinforce = false);
		bool enableEncryptorByPemData(const std::string &PemData, bool packageReinforce = false, bool dataEnhance = false, bool dataReinforce = false);
		bool enableEncryptorByDerFile(const char *derFilePath, bool packageReinforce = false, bool dataEnhance = false, bool dataReinforce = false);
		bool enableEncryptorByPemFile(const char *pemFilePath, bool packageReinforce = false, bool dataEnhance = false, bool dataReinforce = false);
		inline void enableEncryptor(const std::string& curve, const std::string& peerPublicKey, bool packageReinforce = false, bool dataEnhance = false, bool dataReinforce = false);

		//-- Timeout in milliseconds
		virtual FPAnswerPtr sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec = 0);
		virtual bool sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec = 0);

		void setUntransmittedSeconds(int untransmittedSeconds);		//-- 0: using default; -1: disable.
		void setMTU(int MTU) { _MTU = MTU; }		//-- Please called before connect() or sendQuest().

		/*===============================================================================
		  Interfaces for embed mode.
		=============================================================================== */
		/*
		* 如果返回成功，rawData 由 C++ SDK 负责释放(delete), 如果失败，需要调用者释放。
		*/
		virtual bool embed_sendData(std::string* rawData, bool discardable, int timeoutMsec = 0);
		virtual bool embed_sendData(std::string* rawData) { return embed_sendData(rawData, false, 0); }
	};

	typedef std::shared_ptr<UDPClient> UDPClientPtr;

### 创建与构造

UDPClient 的构造函数为私有成员，无法直接调用。请使用静态成员函数

	inline static UDPClientPtr UDPClient::createClient(const std::string& host, int port, bool autoReconnect = true);
	inline static UDPClientPtr UDPClient::createClient(const std::string& endpoint, bool autoReconnect = true);

或基类静态成员函数

	static UDPClientPtr Client::createUDPClient(const std::string& host, int port, bool autoReconnect = true);
	static UDPClientPtr Client::createUDPClient(const std::string& endpoint, bool autoReconnect = true);

创建。

**参数说明**

* **`const std::string& host`**

	服务器 IP 或者域名。

* **`int port`**

	服务器端口。

* **`const std::string& endpoint`**

	服务器 endpoint。

* **`bool autoReconnect`**

	是否自动重连。

### 成员函数

#### asyncConnect

	virtual bool asyncConnect() { return connect(); }

异步连接（伪异步实现）。

**注意**

因为 UDP Client 的异步连接操作，只有**连接建立事件**这一环节需要异步。但一般情况下，几乎不会用到该事件。因此简便起见，`asyncConnect()` 直接调用同步连接方法 `connect()`。如有特殊需求，再改为真异步实现。


#### enableEncryptor

	inline void enableEncryptor(const std::string& curve, const std::string& peerPublicKey, bool packageReinforce = false, bool dataEnhance = false, bool dataReinforce = false);

启用链接加密。

**参数说明**

* **`const std::string& curve`**

	ECDH (椭圆曲线密钥交换) 所用曲线名称。

	可用值：

	+ "secp256k1"
	+ "secp256r1"
	+ "secp224r1"
	+ "secp192r1"

* **`const std::string& peerPublicKey`**

	服务端公钥（二进制数据）。

	**注意**

	该公钥为裸密钥，由 [FPNN 框架](https://github.com/highras/fpnn) 内置工具 [eccKeyMaker](https://github.com/highras/fpnn/blob/master/core/tools/eccKeyMaker.cpp) 生成。

* **`bool packageReinforce = false`**

	UDP 整包加密密钥长度选择：true 表示采用 256 位密钥；false 表示采用 128 位密钥。

* **`bool dataEnhance = false`**

	是否启动数据内容强化加密。

* **`bool dataReinforce = false`**

	数据内容加密密钥长度选择：true 表示采用 256 位密钥；false 表示采用 128 位密钥。

#### enableEncryptorByDerData

	bool enableEncryptorByDerData(const std::string &derData, bool packageReinforce = false, bool dataEnhance = false, bool dataReinforce = false);

启用链接加密。

**参数说明**

* **`const std::string &derData`**

	服务端公钥，DER 格式。

* **`bool packageReinforce = false`**

	UDP 整包加密密钥长度选择：true 表示采用 256 位密钥；false 表示采用 128 位密钥。

* **`bool dataEnhance = false`**

	是否启动数据内容强化加密。

* **`bool dataReinforce = false`**

	数据内容加密密钥长度选择：true 表示采用 256 位密钥；false 表示采用 128 位密钥。

#### enableEncryptorByPemData

	bool enableEncryptorByPemData(const std::string &PemData, bool packageReinforce = false, bool dataEnhance = false, bool dataReinforce = false);

启用链接加密。

**参数说明**

* **`const std::string &PemData`**

	服务端公钥，PEM 格式。

* **`bool packageReinforce = false`**

	UDP 整包加密密钥长度选择：true 表示采用 256 位密钥；false 表示采用 128 位密钥。

* **`bool dataEnhance = false`**

	是否启动数据内容强化加密。

* **`bool dataReinforce = false`**

	数据内容加密密钥长度选择：true 表示采用 256 位密钥；false 表示采用 128 位密钥。

#### enableEncryptorByDerFile

	bool enableEncryptorByDerFile(const char *derFilePath, bool packageReinforce = false, bool dataEnhance = false, bool dataReinforce = false);

启用链接加密。

**参数说明**

* **`const char *derFilePath`**

	存储服务端 DER 格式公钥文件的路径。

* **`bool packageReinforce = false`**

	UDP 整包加密密钥长度选择：true 表示采用 256 位密钥；false 表示采用 128 位密钥。

* **`bool dataEnhance = false`**

	是否启动数据内容强化加密。

* **`bool dataReinforce = false`**

	数据内容加密密钥长度选择：true 表示采用 256 位密钥；false 表示采用 128 位密钥。


#### enableEncryptorByPemFile

	bool enableEncryptorByPemFile(const char *pemFilePath, bool packageReinforce = false, bool dataEnhance = false, bool dataReinforce = false);

启用链接加密。

**参数说明**

* **`const char *pemFilePath`**

	存储服务端 PEM 格式公钥文件的路径。

* **`bool packageReinforce = false`**

	UDP 整包加密密钥长度选择：true 表示采用 256 位密钥；false 表示采用 128 位密钥。

* **`bool dataEnhance = false`**

	是否启动数据内容强化加密。

* **`bool dataReinforce = false`**

	数据内容加密密钥长度选择：true 表示采用 256 位密钥；false 表示采用 128 位密钥。
	

#### sendQuestEx

	virtual FPAnswerPtr sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec = 0);
	virtual bool sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec = 0);
	virtual bool sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec = 0);

发送请求。

**参数说明**

* **`FPQuestPtr quest`**

	请求数据对象。具体请参见 [FPQuest](FPQuest.md)。

* **`bool discardable`**

	数据包是否可丢弃/是否是必达消息。

* **`int timeoutMsec`**

	本次请求的超时设置。单位：**毫秒**。

	**注意**

	如果 `timeoutMsec` 为 0，表示使用 Client 实例当前的请求超时设置。  
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

#### setUntransmittedSeconds

	void setUntransmittedSeconds(int untransmittedSeconds);		//-- 0: using default; -1: disable.

设置连接 idle 超时时间。单位：秒。默认：60 秒。`0` 表示使用默认值，`-1` 表示取消 idle 超时设置，即永远不会因为 idle 导致链接关闭。

#### setMTU

	void setMTU(int MTU); { _MTU = MTU; }		//-- Please called before connect() or sendQuest().

设置当前连接 MTU 大小。默认：对于内网地址为 1500 字节，外网地址为 576 字节。

**注意**

此设置，只有在**任何连接和发送操作**调用前调用，才会生效。否则会使用默认 MTU 大小，且不可更改。

#### embed_sendData

	virtual bool embed_sendData(std::string* rawData, bool discardable, int timeoutMsec = 0);
	virtual bool embed_sendData(std::string* rawData) { return embed_sendData(rawData, false, 0); }

**注意：嵌入模式专用**

用于 Java、Unity、Go、C# 等语言封装 C++ SDK 时，上层语言通过 C++ SDK 直接发送 FPNN 协议 UDP 数据，C++ SDK 层面不再做额外处理。

相较于基类对象，增加 UDP 数据专用的 `embed_sendData(std::string*, bool, int)` 重载。如果通过原始声明发送 UDP 数据，则被视为**不可丢弃**数据，且发送超时采用默认超时。

**参数说明**

* **`std::string* rawData`**

	需要发送的，已经编码完毕，封装完毕的 FPNN 协议数据。

* **`bool discardable`**

	数据包是否可丢弃/是否是必达数据。

* **`int timeoutMsec`**

	本次请求的超时设置。单位：**毫秒**。

	**注意**

	如果 `timeoutMsec` 为 0，表示使用 UDPClient 实例当前的请求超时设置。  
	如果 UDPClient 实例当前的请求超时设置为 0，则使用 ClientEngine 的请求超时设置。

**返回值说明**

返回 true 表示成功，返回 false 表示失败。

**注意**

如果返回 true，`std::string* rawData` 将由 SDK 在合适的时候调用 `delete` 进行释放。  
如果返回 false，使用者需要负责 `std::string* rawData` 对象的释放。
