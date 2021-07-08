## Config

### 介绍

全局配置模块。

**注意：请勿使用非文档化的 API。**  
非文档化的 API，或为内部使用，或因历史原因遗留，或为将来设计，后续版本均可能存在变动。

### SDK 全局相关

* **`std::string Config::_version;`**

	当前 SDK 版本号。

* **`int Config::_max_recv_package_length;`**

	**接收 FPNN 协议数据包**时，接收的最大大小。如果收到的数据包超过该大小，当前链接将被视为错误/恶意链接被强制关闭。默认：8 MB。

### SDK Embed 模式

* **`bool Config::_embed_receiveBuffer_freeBySDK;`**

	数据直收接口 [Client::embed_configRecvNotifyDelegate](Client.md#embed_configRecvNotifyDelegate) 收到的数据，是否由 SDK 释放。  
	具体请参见：[嵌入模式](../embedMode.md)。默认：true

* **`static inline void Config::embed_ConfigReceiveBufferAutoFree(bool autoFree)`**

	配置数据直收接口 [Client::embed_configRecvNotifyDelegate](Client.md#embed_configRecvNotifyDelegate) 收到的数据，是否由 SDK 释放。

	true 为 SDK 释放，false 为调用者释放(free)。

	具体请参见：[嵌入模式](../embedMode.md)。

### TCP 连接自动保活

* **`bool Config::Client::KeepAlive::defaultEnable;`**

	是否所有 TCP 连接，**默认强制开启**(TCPClient 无须调用 `keepAlive()` 接口显式开启)自动保活。默认值：false

* **`int Config::Client::KeepAlive::pingInterval;`**

	TCP 连接，自动保活时，ping 请求间隔时间，单位：**毫秒**。默认：20000 豪秒(20 秒)。

* **`int Config::Client::KeepAlive::maxPingRetryCount;`**

	TCP 连接，自动保活时，ping 超时后，最大重试次数。超过该次数，认为链接丢失。默认：3 次。

### UDP 客户端与 UDP 连接

* **`int Config::UDP::_LAN_MTU;`**

	对于内网，MTU 大小。默认：1500 字节。

* **`int Config::UDP::_internet_MTU;`**

	对于外网，MTU 大小。默认：576 字节。

* **`int Config::UDP::_heartbeat_interval_seconds(20);`**

	UDP 连接，自动保活时，心跳间隔时间，单位：秒。默认：20 秒。

* **`int Config::UDP::_max_cached_uncompleted_segment_package_count;`**

	UDP 连接，待组装的数据包，最大缓存数量限制（针对组装后的完整包数量，非碎片包数量）。默认：100。

* **`int Config::UDP::_max_cached_uncompleted_segment_seconds;`**

	UDP 连接，待组装的碎片包，最大缓存时间。默认：300 秒。

* **`int Config::UDP::_max_untransmitted_seconds;`**

	UDP 连接，链接丢失确认前，等待时间。单位：秒。默认：60 秒。

