## TCPClient

### 介绍

TCP 客户端。[Client](Client.md) 的子类。

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 关键定义

	class TCPClient: public Client, public std::enable_shared_from_this<TCPClient>
	{
	public:
		virtual ~TCPClient();

		/*===============================================================================
		  Call by Developer. Configure Function.
		=============================================================================== */
		inline void enableEncryptor(const std::string& curve, const std::string& peerPublicKey, bool packageMode = true, bool reinforce = false);

		bool enableEncryptorByDerData(const std::string &derData, bool packageMode = true, bool reinforce = false);
		bool enableEncryptorByPemData(const std::string &PemData, bool packageMode = true, bool reinforce = false);
		bool enableEncryptorByDerFile(const char *derFilePath, bool packageMode = true, bool reinforce = false);
		bool enableEncryptorByPemFile(const char *pemFilePath, bool packageMode = true, bool reinforce = false);

		void setKeepAlivePingTimeout(int seconds);
		void setKeepAliveInterval(int seconds);
		void setKeepAliveMaxPingRetryCount(int count);

		inline void setConnectTimeout(int seconds);
		inline int getConnectTimeout();

		inline static TCPClientPtr createClient(const std::string& host, int port, bool autoReconnect = true);
		inline static TCPClientPtr createClient(const std::string& endpoint, bool autoReconnect = true);
	};

	typedef std::shared_ptr<TCPClient> TCPClientPtr;

### 创建与构造

TCPClient 的构造函数为私有成员，无法直接调用。请使用静态成员函数

	inline static TCPClientPtr TCPClient::createClient(const std::string& host, int port, bool autoReconnect = true);
	inline static TCPClientPtr TCPClient::createClient(const std::string& endpoint, bool autoReconnect = true);

或基类静态成员函数

	static TCPClientPtr Client::createTCPClient(const std::string& host, int port, bool autoReconnect = true);
	static TCPClientPtr Client::createTCPClient(const std::string& endpoint, bool autoReconnect = true);

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

#### enableEncryptor

	inline void enableEncryptor(const std::string& curve, const std::string& peerPublicKey, bool packageMode = true, bool reinforce = false);

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

* **`bool packageMode = true`**

	true 为包加密模式，false 为流加密模式。

* **`bool reinforce = false`**

	true 表示采用 256 位密钥；false 表示采用 128 位密钥。

#### enableEncryptorByDerData

	bool enableEncryptorByDerData(const std::string &derData, bool packageMode = true, bool reinforce = false);

启用链接加密。

**参数说明**

* **`const std::string &derData`**

	服务端公钥，DER 格式。

* **`bool packageMode = true`**

	true 为包加密模式，false 为流加密模式。

* **`bool reinforce = false`**

	true 表示采用 256 位密钥；false 表示采用 128 位密钥。

#### enableEncryptorByPemData

	bool enableEncryptorByPemData(const std::string &PemData, bool packageMode = true, bool reinforce = false);

启用链接加密。

**参数说明**

* **`const std::string &PemData`**

	服务端公钥，PEM 格式。

* **`bool packageMode = true`**

	true 为包加密模式，false 为流加密模式。

* **`bool reinforce = false`**

	true 表示采用 256 位密钥；false 表示采用 128 位密钥。

#### enableEncryptorByDerFile

	bool enableEncryptorByDerFile(const char *derFilePath, bool packageMode = true, bool reinforce = false);

启用链接加密。

**参数说明**

* **`const char *derFilePath`**

	存储服务端 DER 格式公钥文件的路径。

* **`bool packageMode = true`**

	true 为包加密模式，false 为流加密模式。

* **`bool reinforce = false`**

	true 表示采用 256 位密钥；false 表示采用 128 位密钥。


#### enableEncryptorByPemFile

	bool enableEncryptorByPemFile(const char *pemFilePath, bool packageMode = true, bool reinforce = false);

启用链接加密。

**参数说明**

* **`const char *pemFilePath`**

	存储服务端 PEM 格式公钥文件的路径。

* **`bool packageMode = true`**

	true 为包加密模式，false 为流加密模式。

* **`bool reinforce = false`**

	true 表示采用 256 位密钥；false 表示采用 128 位密钥。

#### setKeepAlivePingTimeout

	void setKeepAlivePingTimeout(int seconds);

设置自动保活状态下的 ping 请求超时时间，单位：秒。默认：5 秒。

#### setKeepAliveInterval

	void setKeepAliveInterval(int seconds);

设置自动保活状态下的 ping 请求间隔时间，单位：秒。默认：20 秒。

#### setKeepAliveMaxPingRetryCount

	void setKeepAliveMaxPingRetryCount(int count);

设置开启自动保活时，ping 超时后，最大重试次数。超过该次数，认为链接丢失。默认：3 次。

#### setConnectTimeout

	inline void setConnectTimeout(int seconds);

设置当前 Client 实例的连接超时。单位：秒。`0` 表示使用 ClientEngine 的连接超时设置。

#### getConnectTimeout

	inline int getConnectTimeout();

获取当前 Client 实例的连接超时设置。
