## Configurations

### 编译配置

#### Wextra 选项

需要使用 `-Wextra` 编译选项时，取消 [def.mk](../src/def.mk) 中，含有 `-Wextra` 一行的 "CPPFLAGS" 的注释即可。

如果在使用 SDK 库文件的过程中需要使用 `-Wextra` 编译选项，请视情况增加编译选项 `-Wno-unused-parameter -Wno-implicit-fallthrough`。

### SDK 配置

全局配置：

+ 静态配置：

	请参见 [Config](APIs/Config.md) 相关参数。

+ 动态配置：

	请参见 [ClientEngine](APIs/ClientEngine.md) 相关接口。

TCP & UDP 客户端：

+ 动态配置：

	通用配置请参见 [Client](APIs/Client.md) 相关接口；

	TCP 特有配置请参见 [TCPClient](APIs/TCPClient.md) 相关接口；

	UDP 特有配置请参见 [UDPClient](APIs/UDPClient.md) 相关接口。
