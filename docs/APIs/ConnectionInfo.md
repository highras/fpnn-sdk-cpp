## ConnectionInfo

### 介绍

ConnectionInfo 连接信息对象。

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### 关键定义

	class ConnectionInfo
	{
	public:
		int socket;
		int port;
		std::string ip;

		std::string str() const;
		std::string endpoint() const;

		inline bool isTCP() const;
		inline bool isUDP() const;
		inline bool isEncrypted() const;
		inline bool isPrivateIP() const;
		inline uint64_t uniqueId();

		ConnectionInfo(const ConnectionInfo& ci);

		~ConnectionInfo();
	};

	typedef std::shared_ptr<ConnectionInfo> ConnectionInfoPtr;

### 构造函数

	ConnectionInfo(const ConnectionInfo& ci);

ConnectionInfo 对象无法直接生成。

**拷贝构造函数**生成的副本，仅供业务缓存和跟踪使用。SDK 内部相关接口，大部分需要使用原本实例。

### 公有域

#### socket

Socket 套接字。

#### port

端口。

#### ip

域名解析后的 IP 地址。

### 成员函数

#### str

	std::string str() const;

返回 socket 与 endpoint 组成的字符串。

#### endpoint

	std::string endpoint() const;

Endpoint 接入点。

#### isTCP

	inline bool isTCP() const;

是否是 TCP 连接。

#### isUDP

	inline bool isUDP() const;

是否是 UDP 连接。

#### isEncrypted

	inline bool isEncrypted() const;

是否是加密链接。

#### isPrivateIP

	inline bool isPrivateIP() const;

是否是内网链接。

#### uniqueId

	inline uint64_t uniqueId();

返回**对应连接**的**全局唯一**的 id。