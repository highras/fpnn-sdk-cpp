## 样例模块

**注意**

* 所有演示用例，均使用 [FPNN 框架](https://github.com/highras/fpnn)默认的目标测试服务器[serverTest](https://github.com/highras/fpnn/blob/master/core/test/UniversalFunctionalTests/serverTest.cpp)。


### 测试模块

样例模块位置：[/examples/](../examples/)

* **aheadAnswerInDuplex**

	在 Server Push/Duplex 的处理中，提前返回的用法示范。

		Usage: ./aheadAnswerInDuplex-tcp <endpoint> [-udp]

	示范内容：

	+ Server Push 的处理
	+ Server Psuh 处理函数的注册
	+ 在 Server Psuh 处理函数中，在函数返回前，提前返回请求应答
	+ 同步发送请求

* **asyncAnswerInDuplex**

	在 Server Push/Duplex 的处理中，异步返回的用法示范。

		Usage: ./asyncAnswerInDuplex-tcp <endpoint> [-udp]

	示范内容：

	+ Server Push 的处理
	+ Server Psuh 处理函数的注册
	+ 在 Server Psuh 处理函数中，异步返回请求应答
	+ SDK 自带线程池/任务池的使用
	+ 同步发送请求

* **connectionEvents**

	连接事件用法示范。

		Usage: ./connectionEvents-tcp <endpoint> [-udp]

	示范内容：

	+ 链接建立事件的用法
	+ 链接即将关闭事件的用法
	+ 异步(lambda)发送请求
	+ 同步发送请求


* **encryptedCommunication**

	加密链接的使用示范。

		Usage: ./encryptedCommunication-tcp <endpoint> <server-pem-key-file> [-udp]

	示范内容：

	+ 加密链接的启用
	+ 异步(lambda)发送请求
	+ 同步发送请求


* **oneWayDuplex**

	单向(oneway) Server Push/Duplex 的请求处理示范。

		Usage: ./oneWayDuplex-tcp <endpoint> [-udp]

	示范内容：

	+ 单向(oneway) Server Push 的处理
	+ Server Psuh 处理函数的注册
	+ 同步发送请求


* **twoWayDuplex**

	双向(twoway) Server Push/Duplex 的请求处理示范。

		Usage: ./twoWayDuplex-tcp <endpoint> [-udp]

	示范内容：

	+ 双向(twoway) Server Push 的处理
	+ Server Psuh 处理函数的注册
	+ 同步发送请求