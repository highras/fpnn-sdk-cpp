#ifndef FPNN_CPP_SDK_EMBEDDED_TYPE_H
#define FPNN_CPP_SDK_EMBEDDED_TYPE_H

/*
*	Embed 注意事项：
*
*	0. **强烈建议** 上层语言将域名解析成IP(IPv4/IPv6)后，再传入C++ SDK层。否则创建 Client(TCPClient/UDPClient)
*	   时，C++ SDK 会去执行DNS 解析。但目前DNS解析在各平台没有通用的异步解析接口。因此此时的DNS解析。将会调用同步接口，
*	   因此极有可能导致上层调用的阻塞。
*	
*	1. 发送数据(FPNN 的二进制数据)请使用 Client/TCPClient/UDPClient 的 embed_sendData() 接口；
*	2. Client/TCPClient/UDPClient 的 embed_sendData() 接口为异步接口，同步发送需要上层封装。上层封装可参见
*	   C++/C# 的 sendQuest 的同步接口实现，以及对应的 SyncedAnswerCallback 实现。
*	3. embed_sendData() 接口，如果返回成功，参数 rawData 由 C++ SDK 负责释放(delete), 如果失败，需要调用者释放。
*	4. EmbedRecvNotifyDelegate 的 buffer，默认情况下调用者无需分配或者释放。
*	   如果调用 Config::_embed_receiveBuffer_freeBySDK(bool autoFree)，设置参数 autoFree = false，
*	   buffer 需要由调用者释放（free）。
*	   注意：目前仅对 TCP 有效，UDP 均由 SDK 释放。
*	
*	极端情况：
*	5. 由于线程切换和操作系统线程调度的缘故，存在着 Client 释放后的极短时间内，对应的 EmbedRecvNotifyDelegate 依然
*	   被调用的情况(概率极低)。为了防止以上小概率事件发生，可参考以下两个方案：
*		i. EmbedRecvNotifyDelegate 不与上层语言对象绑定，而是使用全局静态函数实现；
*		ii. 若 EmbedRecvNotifyDelegate 与上层对象绑定，如 上层语言的 Client 对象实例。则，建议在上层对象释放时，
*			先释放 C++ SDK 对应的 client 实例，然后 sleep 5 ms（具体时间取决于 EmbedRecvNotifyDelegate 函数的
*			最慢执行耗时。一般 5 毫秒能覆盖大多数情况。）。如果担心 sleep 5 ms 导致当前线程占用，建议可以将其加入上层
*			业务的资源回收队列，释放 C++ SDK 的 client 实例后，再根据具体情况，异步回收上层语言与 EmbedRecvNotifyDelegate
*			相关的资源。
*/
namespace fpnn
{
	/*
	*	默认 buffer 由 SDK 维护，不需要调用者管理释放。
	*	如果 Config::_embed_receiveBuffer_freeBySDK(bool autoFree) 参数 autoFree = false，
	*	buffer 需要由调用者释放（free）。
	*	注意：目前仅对 TCP 有效，UDP 均由 SDK 释放。
	*/
	typedef void (*EmbedRecvNotifyDelegate)(uint64_t connectionId, const void* buffer, int length);
}

#endif