# FPNN C++ SDK

## 介绍

FPNN C++ SDK 是独立于 FPNN Framework 的跨平台客户端 SDK。

主要供不使用 FPNN Framework 的 C++ 程序使用。如果应用/服务已使用 FPNN Framework，请直接使用 FPNN Framework 的客户端部分，无需再次使用 FPNN C++ SDK。

本 SDK 提供的客户端为 IO 操作聚合的客户端，并非双线程客户端。因此配置完毕后，可以同时运行数百个客户端实例，而无需担心线程暴涨。

**注意：因实现机制不同，FPNN Framework 客户端部分可以同时运行数万客户端实例，而 SDK 部分不建议超过 1024 个客户端实例。**

### 支持特性

* 支持 IPv4
* 支持 IPv6
* 支持 msgPack 编码
* 支持 json 编码
* 支持 可选参数
* 支持 不定类型/可变类型参数
* 支持 不定长度不定类型参数
* 支持 接口灰度兼容
* 支持 Server Push
* 支持 异步调用
* 支持 同步调用
* 支持 应答提前返回
* 支持 应答异步返回
* TCP：支持加密链接
* TCP：支持自动保活
* UDP：支持自动保活
* UDP：支持可靠连接
* UDP：支持可丢弃数据和不可丢弃数据混发
* UDP：支持乱序重排
* UDP：支持大数据自动切割 & 自动组装
* UDP：支持零散数据合并发送
* UDP：支持毫秒级超时控制


### 适配平台

| 操作系统 | 编译器 |
|---------|-------|
| CentOS 7 | GCC/G++ 4.8.5 |
| CentOS 8 | GCC/G++ 8 |
| Ubuntu 20 | GCC/G++ 9 |
| MaxOS | Apple clang 12.0.0 |
| iOS | XCode |
| Android (NDK) | Android Studio |

**暂未适配 Windows 平台**


## 使用

### 安装

**本 SDK 使用 C++11 编写，编译器需要支持 C++11 语法。g++ 版本建议使用 4.8.5 或以上。**

1. 编译

		cd <fpnn-C++-SDK-folder>
		make

1. release

		sh release.sh

### 开发 & 使用

* [SDK使用向导](docs/guide.md)
* [样例演示](docs/examples.md)
* [API手册](docs/API.md)
* [嵌入模式](docs/embedMode.md)
* [SDK配置](docs/config.md)
* [内置工具](docs/tools.md)
* [测试介绍](docs/tests.md)
* [错误代码](docs/errorCode.md)


### 目录结构

* **\<fpnn-C++-SDK-folder\>/src**

	SDK 核心代码。

* **\<fpnn-C++-SDK-folder\>/docs**

	SDK 文档目录。

* **\<fpnn-C++-SDK-folder\>/examples**

	SDK 主要功能演示。

* **\<fpnn-C++-SDK-folder\>/tests**

	测试程序代码。

* **\<fpnn-C++-SDK-folder\>/tools**

	SDK 内置工具。

