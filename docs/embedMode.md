## Embed Mode

嵌入模式供 Java、Unity、Go、C# 等语言封装 C++ SDK 使用。

在 Java、Unity、Go、C# 等语言封装 C++ SDK 的时候，一般会面临以下诸多复杂度极高的问题：

* 封装语言调用 C++ 的 API
* 封装语言设置回调给 C++
* C++ 调用封装语言的回调
* 封装语言传递 lambda 表达式给 C++
* C++ 持有封装语言的 lambda
* C++ 调用封装语言的 lambda
* 封装语言释放 C++ 分配的对象
* C++ 释放封装语言分配的对象
* 封装语言的对象生命周期和操作与 C++ 对象的证明周期和操作绑定，二者同步一致
* 在高频度操作下，封装语言和 C++ 之间的内存管理最优化

为了规避，或者降低以上问题的复杂度，为了打断封装语言和 C++ 之间，lambda、内存对象、回调 等的绑定和来回穿透，创建者和释放者不一致等问题，C++ SDK 提供嵌入模式。

在此模式下，C++ SDK 提供 FPNN 协议二进制数据的直收/直发功能，不再对数据进行解包和封装，且不再管理封装语言的回调和同步操作。

因此，封装语言需要管理请求对应的回调，以及相关的超时检查，和同步操作封装。

具体例子，可参考：[以C++为例，C++ 封装 C++ SDK 嵌入式模式](../tests/embedModeTests/DemoBridgeClient.h)。

其他语言，可用对应的功能，将上层 C++ 封装直接翻译即可（强烈建议根据具体语言特性进行结构和逻辑的优化）。

### 核心接口

* **直发接口**

		virtual bool Client::embed_sendData(std::string* rawData);

	具体请参考：[embed_sendData](APIs/Client.md#embed_sendData)。

* **直收接口**

	* **直收接口原型**

			void EmbedRecvNotifyDelegate(uint64_t connectionId, const void* buffer, int length);

		具体请参见 [embedTypes.h](../src/core/embedTypes.h)。

	* **直收接口设置接口**

			virtual void Client::embed_configRecvNotifyDelegate(EmbedRecvNotifyDelegate delegate);

		具体请参考：[embed_configRecvNotifyDelegate](APIs/Client.md#embed_configRecvNotifyDelegate)。


### 注意事项

* **强烈建议** 封装语言将域名解析成IP(IPv4/IPv6)后，再传入C++ SDK层。否则创建 Client(TCPClient/UDPClient) 时，C++ SDK 会去执行 DNS 解析。但目前 DNS 解析在各平台没有通用的异步解析接口。因此此时的 DNS 解析。将会调用同步接口，因此极有可能导致封装层调用的阻塞。

* Client/TCPClient/UDPClient 的 embed_sendData() 接口为异步接口，同步发送需要上层封装。上层封装可参见：

	+ C++： [SyncedAnswerCallback](../src/core/AnswerCallbacks.h)
	+ C#：  [SyncAnswerCallback](https://github.com/highras/fpnn-sdk-csharp/blob/master/fpnn-sdk/AnswerCallback.cs)
	+ Java：[SyncAnswerCallback](https://github.com/highras/fpnn-sdk-java/blob/master/src/main/java/com/fpnn/sdk/SyncAnswerCallback.java)

* embed_sendData() 接口，如果返回成功，参数 rawData 由 C++ SDK 负责释放(delete), 如果失败，需要调用者释放。
* EmbedRecvNotifyDelegate 的 buffer，默认情况下调用者无需分配或者释放。

	如果调用 `Config::_embed_receiveBuffer_freeBySDK(bool autoFree)`，设置参数 autoFree = false，则 buffer 需要由调用者释放（free）。

* **极端情况**：由于线程切换和操作系统线程调度的缘故，存在着 Client 释放后的极短时间内，对应的 EmbedRecvNotifyDelegate 依然被调用的情况(概率极低)。为了防止以上小概率事件发生，可参考以下两个方案：

	1. EmbedRecvNotifyDelegate 不与封装层语言对象绑定，而是使用全局静态函数实现；
	1. 若 EmbedRecvNotifyDelegate 与封装层对象绑定，如 封装层语言的 Client 对象实例。则，建议在上层对象释放时，先释放 C++ SDK 对应的 client 实例，然后 sleep 5 ms（具体时间取决于 EmbedRecvNotifyDelegate 函数的最慢执行耗时。一般 5 毫秒能覆盖大多数情况。）。如果担心 sleep 5 ms 导致当前线程占用，建议可以将其加入封装层业务的资源回收队列，释放 C++ SDK 的 client 实例后，再根据具体情况，异步回收封装层语言与 EmbedRecvNotifyDelegate 相关的资源。

* **极端情况**：对于 TCPClient，如果开启 keepAlive，则在最后的业务 answer 收到后，每隔一个设置的 KeepAliveInterval，EmbedRecvNotifyDelegate 会收到一个无效的 answer。一般这个 answer 是 C++ SDK TCPClient 底层的 `*ping` 指令的回包。忽略即可。其他情况下收到的无效的 answer，一般伴随着业务请求的超时。根据业务具体情况处理即可。

