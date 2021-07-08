## Configurations

### 配置

全局配置：

+ 静态配置：

	请参见 [Config](APIs/Config.md) 相关参数。

+ 动态配置：

	请参见 [ClientEngine](ClientEngine.md) 相关接口。

TCP & UDP 客户端：

+ 动态配置：

	通用配置请参见 [Client](Client.md) 相关接口；

	TCP 特有配置请参见 [TCPClient](TCPClient.md) 相关接口；

	UDP 特有配置请参见 [UDPClient](UDPClient.md) 相关接口。

### 当前 UDP 配置性能基准

因为 UDP 的相关参数在未来的版本中可能存在不确定的变动，因此未完全文档化。

当前的 UDP 参数测试结果，保证：

* 当单链接 QPS 为 100 时，10 个 UDP 虚拟链接，总 1000 QPS，从宁夏经过GFW访问欧洲和美洲无压力，CPU 在50% 以下。
* 当单链接 QPS 为 50 时，20 个 UDP 虚拟链接，总 1000 QPS，从宁夏经过GFW访问欧洲和美洲无压力，CPU 在50% 以下。
