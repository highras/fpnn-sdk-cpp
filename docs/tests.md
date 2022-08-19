## 测试模块

**注意**：所有测试用例，均使用 [FPNN 框架](https://github.com/highras/fpnn)默认的目标测试服务器[serverTest](https://github.com/highras/fpnn/blob/master/core/test/UniversalFunctionalTests/serverTest.cpp)。

### 测试模块

测试模块位置：[/tests/](../tests/)

* **asyncStressClient**

	异步压力测试。

		Usage: ./asyncStressClient ip port connections total-qps [-udp]

* **clientAsyncOnewayTest**

	Oneway 消息压力测试。

		Usage: ./clientAsyncOnewayTest ip port [-udp]

* **duplexClientTest**

	Server Push/Duplex 压力测试。

		Usage: ./duplexClientTest ip port threadNum sendCount [-udp]

* **periodClientTest**

	周期性发送测试。

		Usage: ./periodClientTest ip port quest_period(seconds) [-udp] [-keepAlive]

* **timeoutTest**

	超时控制测试。

		Usage: ./timeoutTest ip port delay_seconds [-e engine_quest_timeout] [-c client_quest_timeout] [-q single_quest_timeout] [-udp]

* **singleClientConcurrentTest**

	单一客户端多线程并发稳定性测试。

		Usage: ./singleClientConcurrentTest ip port [-ecc-pem ecc-pem-file [-package|-stream] [-128bits|-256bits]]
		Usage: ./singleClientConcurrentTest ip port -udp [-ecc-pem ecc-pem-file [-packageReinforce] [-dataEnhance [-dataReinforce]]]


### 嵌入模式测试模块

测试模块位置：[/tests/embedModeTests/](../tests/embedModeTests/)

* **embedAsyncStressClient**

	异步压力测试。

		Usage: ./embedAsyncStressClient ip port connections total-qps [-ecc-pem ecc-pem-file] [-udp]

* **embedAsyncStressClient**

	Oneway 消息压力测试。

		Usage: ./embedAsyncStressClient ip port [-udp]

* **embedDuplexClientTest**

	Server Push/Duplex 压力测试。

		Usage: ./embedDuplexClientTest ip port threadNum sendCount [-udp]

* **embedPeriodClientTest**

	周期性发送测试。

		Usage: ./embedPeriodClientTest ip port quest_period(seconds) [-udp] [-keepAlive]

* **embedTimeoutTest**

	超时控制测试。

		Usage: ./embedTimeoutTest ip port delay_seconds [-e engine_quest_timeout] [-c client_quest_timeout] [-q single_quest_timeout] [-udp]

* **embedSingleClientConcurrentTest**

	单一客户端多线程并发稳定性测试。

		Usage: ./embedSingleClientConcurrentTest ip port [-ecc-pem ecc-pem-file] [-udp]