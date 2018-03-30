# FPNN C++ SDK

**本文档为临时文档，内容从简，未来进一步补全**

## 一、介绍

FPNN C++ SDK 是独立于 FPNN Framework 的跨平台客户端 SDK。

主要供不使用 FPNN Framework 的 C++ 程序使用。如果应用/服务已使用 FPNN Framework，请直接使用 FPNN Framework 的客户端部分，无需再次使用 FPNN C++ SDK。

本 SDK 提供的客户端为 IO 操作聚合的客户端，并非双线程客户端。因此配置完毕后，可以同时运行数百个客户端实例，而无需担心线程暴涨。

**注意：因实现机制不同，FPNN Framework 客户端部分可以同时运行数万客户端实例，而 SDK 部分不建议超过 1024 个客户端实例。**


### 安装

+ 本 SDK 使用 C++11 编写，编译器需要支持 C++11 语法。g++ 版本建议使用 4.8.5 或以上。
+ 本 SDK 部分功能依赖于 [libcurl](https://curl.haxx.se/libcurl/)

1. 编译

		cd <fpnn-C++-SDK-folder>
		make

1. release

		sh release.sh

### 开发

**使用**

FPNN C++ SDK 的使用除少部分细节外，与 FPNN Framework 客户端部分无异。具体可参考：

[FPNN Framework 客户端基础使用说明](https://github.com/highras/fpnn/blob/master/doc/zh-cn/fpnn-client-basic-tutorial.md)

[FPNN Framework 客户端高级使用说明](https://github.com/highras/fpnn/blob/master/doc/zh-cn/fpnn-client-advanced-tutorial.md)

**编译**

如果没有执行 release，请参考 <fpnn-C++-SDK-folder>/tools/Makefile 或 <fpnn-C++-SDK-folder>/tests/Makefile

如果已经执行 release，还可以参考 <fpnn-C++-SDK-folder>/tools/Makefile2


### 目录结构

* **<fpnn-C++-SDK-folder>/src**

	SDK 核心代码

* **<fpnn-C++-SDK-folder>/examples**

	SDK 主要功能演示

* **<fpnn-C++-SDK-folder>/tools**

	FPNN Framework 内置工具。可参见：[FPNN Framework 内置工具](https://github.com/highras/fpnn/blob/master/doc/zh-cn/fpnn-tools.md) “运维/管理工具”部分。

* **<fpnn-C++-SDK-folder>/test**

	并发测试和稳定性测试程序代码。


### 平台相关

目前 FPNN C++ SDK 适配的平台为：

+ CentOS
+ Ubuntu
+ MacOS X
+ iOS
