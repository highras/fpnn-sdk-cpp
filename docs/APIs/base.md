## base library

### 基础库介绍

base 基础库，为 SDK 所依赖的内嵌通用基础库。SDK使用者可直接调用。

功能列表：

| 功能 | 描述 | 头文件 |
|-----|-----|-------|
| 自动执行或释放 | 超出作用域后自动执行或释放 | AutoRelease.h |
| 加密和编码 | 编码 | base64.h |
| 加密和编码 | 编码 | md5.h |
| 加密和编码 | 编码 | sha1.h |
| 加密和编码 | 编码 | sha256.h |
| 加密和编码 | 加密 | rijndael.h |
| 命令行解析 | 命令行解析 | CommandLineUtil.h |
| 大小端处理 | 平台兼容文件 | Endian.h |
| 文件系统 | 常见文件操作封装 | FileSystemUtil.h |
| Json | Json 编解码 | FPJson.h |
| 日志 | 日志记录 | FPLog.h |
| 错误 & 异常 | 错误 & 异常定义 | FpnnError.h |
| Hash | hash 函数对象 | HashFunctor.h |
| Hash | hash 函数 | hashint.h |
| Hash | jenkins 算法 | jenkins.h |
| 16 进制化 | 16 进制化 | hex.h |
| 字符串工具 | 字符串工具 | StringUtil.h |
| HTTP Code 查询 | HTTP Code 含义查询 | httpcode.h |
| 信号处理 | 批量忽略常见信号 | ignoreSignals.h |
| 线程池 | 接口定义 | ITaskThreadPool.h |
| 线程池 | 线程池 | TaskThreadPool.h |
| 数据容器 | LRU map | LruHashMap.h |
| 时间处理 | 兼容文件 | msec.h |
| 时间处理 | 时间格式化 & 兼容处理 | TimeUtil.h |
| 网络工具 | 网络工具 | NetworkUtility.h |


** 注意 **

* C++ SDK 的base 库为 [FPNN][] 框架对应 base 库的缩减版和修改版，绝大部分内容可在 [FPNN][] 框架下直接使用。

* 以上列出内容可直接使用，未列出内容会随 SDK 变动而进行变更，因此不建议使用未列出内容。

* 其中部分代码来源于公开网络，使用 BSD 或者 MIT 许可证。具体来源和许可证，可参考文件头部注释。


### [AutoRelease.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/AutoRelease.h)

类似于 Go 的 defer。  
但 Go 的 defer 是在函数退出前执行，而 AutoRelease 系列是在作用域(函数/代码块)结束时执行。

#### AutoDeleteGuard

	template <typename K>
	class AutoDeleteGuard
	{
	public:
		AutoDeleteGuard(K* naked_ptr);
		~AutoDeleteGuard();
		void release();
	};

当超出作用域时，自动调用 delete 释放初始化时传入的已经 new 出来的对象。

* **void release()**

	取消自动执行。

#### AutoDeleteArrayGuard

	template <typename K>
	class AutoDeleteArrayGuard
	{
	public:
		AutoDeleteArrayGuard(K* naked_ptr);
		~AutoDeleteArrayGuard();
	};

当超出作用域时，自动调用 delete [] 释放初始化时传入的已经 new 出来的**对象数组**。

#### AutoFreeGuard

	class AutoFreeGuard
	{
	public:
		AutoFreeGuard(void* naked_ptr);
		~AutoFreeGuard();
	};

当超出作用域时，自动调用 free 释放初始化时传入的已经 malloc 出来的内存块。

#### PointerContainerGuard

	template <template <typename, typename> class Container, typename Element, class Alloc = std::allocator<Element*>>
	class PointerContainerGuard
	{
	public:
		PointerContainerGuard(Container<Element*, Alloc>* container, bool deleteCounter = false);
		~PointerContainerGuard();
		void release();
	};

适用于指向指针容器的容器指针对象。  
适用容器对象为：vector、stack、list、deque、queue 等，不适用于 set 和 map。  
当超出作用域时，自动调用将遍历容器内的指针，逐一调用 delete 进行释放，然后调用 delete 释放容器。

* **void release()**

	取消自动执行。

#### FinallyGuard

	class FinallyGuard
	{
	public:
		explicit FinallyGuard(std::function<void ()> function);
		virtual ~FinallyGuard();
	};

当超出作用域时，自动执行 lambda 函数。

#### ConditionalFinallyGuard

	template <typename K>
	class ConditionalFinallyGuard
	{
	public:
		explicit ConditionalFinallyGuard(std::function<void (const K& cond)> function, const K& defaultCondition);
		virtual ~ConditionalFinallyGuard();
		void changeCondition(const K& condition);
	};

当超出作用域时，自动使用最后设置的 condition 参数执行 lambda 函数。

* **void changeCondition(const K& condition)**

	更改自动执行时的 condition 参数。 

#### CannelableFinallyGuard

	class CannelableFinallyGuard
	{
	public:
		explicit CannelableFinallyGuard(std::function<void ()> function);
		virtual ~CannelableFinallyGuard();
		void cancel();
	};

当超出作用域时，自动执行 lambda 函数。中途可调用 `cancel()` 方法取消执行。

* **void cancel()**

	取消自动执行。

### [base64.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/base64.h)

* **flag 枚举**

		enum {
			BASE64_NO_PADDING = 0x01,
			BASE64_AUTO_NEWLINE = 0x02,
			BASE64_IGNORE_SPACE = 0x10,
			BASE64_IGNORE_NON_ALPHABET = 0x20,
		};

* **`int base64_init(base64_t *b64, const char *alphabet)`**

	初始化 `base64_t *b64` 对象。

	+ **参数**

		* `const char *alphabet`

			使用**内置**常量 `std_base64.alphabet` 或者 `url_base64.alphabet` 。

	+ **返回值**

		0 表示成功，-1 表示失败。


* **`ssize_t base64_encode(const base64_t *b64, char *out, const void *in, size_t len, int flag)`**

	使用 base64 对 `in` 指向的内存块编码，并输出到 `out` 指向的内存块。

	+ **参数**

		* `const base64_t *b64`

			使用 `base64_init()` 初始化的 base64_t 对象。

		* `char *out`

			输出内存。

		* `const void *in`

			输入内存。

		* `size_t len`

			输入字节长度。

		* `int flag`

			使用前面列出的 flag 枚举参数。

	+ **返回值**

		实际输出的字节数。

* **`ssize_t base64_decode(const base64_t *b64, void *out, const char *in, size_t len, int flag)`**

	使用 base64 对 `in` 指向的内存块解码，并输出到 `out` 指向的内存块。

	+ **参数**

		* `const base64_t *b64`

			使用 `base64_init()` 初始化的 base64_t 对象。

		* `void *out`

			输出内存。

		* `const char *in`

			输入内存。

		* `size_t len`

			输入字节长度。

		* `int flag`

			使用前面列出的 flag 枚举参数。

	+ **返回值**

		实际输出的字节数。  
		如果发生错误，返回负值。其绝对值是实际需要的输出内存大小。


### [CommandLineUtil.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/CommandLineUtil.h)

#### CommandLineParser

	class CommandLineParser
	{
	public:
		static void init(int argc, const char* const * argv, int beginIndex = 1);

		static std::string getString(const std::string& sign, const std::string& dft = std::string());
		static intmax_t getInt(const std::string& sign, intmax_t dft = 0);
		static bool getBool(const std::string& sign, bool dft = false);
		static double getReal(const std::string& sign, double dft = 0.0);
		static bool exist(const std::string& sign);

		static std::vector<std::string> getRestParams();
	};

命令行解析工具。  
支持以 `-` 或 `--` 为前导的参数(标记或键值对)和无前导标志的参数混合，支持变长参数。

**注意：**

* 不建议无前导标志的参数跟随在有前导标志的标记(没有值的参数)后，否则会被作为该标记的值，而非无前导标志的参数。

* **`static void init(int argc, const char* const * argv, int beginIndex = 1)`**

	初始化函数。

	**参数说明**

	+ `int argc`

		同 main() 函数的 argc 参数。直接传递即可。

	+ `const char* const * argv`

		同 main() 函数的 argv 参数。直接传递即可。

	+ `int beginIndex`

		需要处理的参数的起始位置。该参数可以指定跳过预定个数的特定参数。

		例：

			example_exe argv1, argv2, argv3, ...

		如果 argv1、argv2 有固定用途，则指定 `beginIndex = 3` 则会从 argv3 开始处理，而忽略 argv1 和 argv2。  
		此时调用 `getRestParams()` 获取的无前导参数列表，也不包含 argv1 和 argv2。

* **`static std::string getString(const std::string& sign, const std::string& dft = std::string())`**

	以字符串形式，获取 `sign` 参数指定的标记的值。如果指定标记不存在，则使用 `dft` 参数作为替代。

* **`static intmax_t getInt(const std::string& sign, intmax_t dft = 0)`**

	以整型形式，获取 `sign` 参数指定的标记的值。如果指定标记不存在，则使用 `dft` 参数作为替代。

* **`static bool getBool(const std::string& sign, bool dft = false)`**

	以布尔值形式，获取 `sign` 参数指定的标记的值。如果指定标记不存在，则使用 `dft` 参数作为替代。

* **`static double getReal(const std::string& sign, double dft = 0.0)`**

	以双精度浮点型形式，获取 `sign` 参数指定的标记的值。如果指定标记不存在，则使用 `dft` 参数作为替代。

* **`static bool exist(const std::string& sign)`**

	检查由 `sign` 参数指定的标记是否存在。

* **`static std::vector<std::string> getRestParams()`**

	获取无前导标志的参数列表。


### [Endian.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/Endian.h)

大小端转换的平台兼容文件。 为没有 `endian.h` 库的平台提供兼容 `endian.h` 库的相关函数。

具体内容可参阅 `endian.h` 库的 man 手册。  

### [FileSystemUtil.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/FileSystemUtil.h)

* **`struct FileAttrs`**

		namespace FileSystemUtil
		{
			struct FileAttrs{
				std::string name;
				std::string sign;
				std::string content;
				std::string ext;
				int32_t size;
				int32_t atime;
				int32_t mtime;
				int32_t ctime;
			};
		}

	+ `name`

		文件名。

	+ `sign`

		以大写16进制表示的文件 md5 值。

	+ `content`

		文件内容。

	+ `ext`

		文件扩展名。

	+ `size`

		文件大小。

	+ `atime`

		文件访问时间，

	+ `mtime`

		文件修改时间。

	+ `ctime`

		文件创建时间。


* **`bool FileSystemUtil::fetchFileContentInLines(const std::string& filename, std::vector<std::string>& lines, bool ignoreEmptyLine = true, bool trimLine = true)`**

	读取 `const std::string& filename` 参数指定的文件内容，并按行存储到 `std::vector<std::string>& lines` 参数中。

* **`bool FileSystemUtil::readFileContent(const std::string& file, std::string& content)`**

	读取 `const std::string& file` 参数指定的文件内容到 `std::string& content` 参数中。

* **`bool FileSystemUtil::saveFileContent(const std::string& file, const std::string& content)`**

	将 `const std::string& content` 参数中的内容，存储到 `const std::string& file` 参数指定的文件中。如果文件已存在，则覆盖原始文件。

* **`bool FileSystemUtil::appendFileContent(const std::string& file, const std::string& content)`**

	将 `const std::string& content` 参数中的内容，追加存储到 `const std::string& file` 参数指定的文件中。

* **`bool FileSystemUtil::readFileAttrs(const std::string& file, FileAttrs& attrs)`**

	读取 `const std::string& file` 参数指定的文件**属性**到 `FileAttrs& attrs` 中（仅属性被读取，**不含文件内容**）。

* **`bool FileSystemUtil::setFileAttrs(const std::string& file, const FileAttrs& attrs)`**

	通过 `FileAttrs& attrs` 设置 `const std::string& file` 参数指定的文件属性。

* **`bool FileSystemUtil::getFileNameAndExt(const std::string& file, std::string& name, std::string& ext)`**

	将 `const std::string& file` 指定的文件路径名，拆解为文件名和文件扩展名，并存储到 `std::string& name` 参数和 `std::string& ext` 参数中。

* **`bool FileSystemUtil::readFileAndAttrs(const std::string& file, FileAttrs& attrs)`**

	读取 `const std::string& file` 参数指定的文件**内容和属性**到 `FileAttrs& attrs` 中。


### [FPJson.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/FPJson.h)

超级便捷的 JSON 编解码模块。  
仅支持 UTF-8 编码。支持 UTF-16 代理对。不支持 BOM 头。

具体参见：[FPJson](FPJson.md)


### [FPLog.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/FPLog.h)

#### FPLog

与 [FPNN][] 框架兼容，为 SDK 特化的日志模块。

	class FPLog;
	typedef std::shared_ptr<FPLog> FPLogPtr;

	class FPLog
	{
	public:
		typedef enum {FP_LEVEL_FATAL=0, FP_LEVEL_ERROR=1, FP_LEVEL_WARN=2, FP_LEVEL_INFO=3, FP_LEVEL_DEBUG=4} FPLogLevel;

	public:
		~FPLog();
		
		static FPLogPtr instance();
		static void log(FPLogLevel curLevel, bool compulsory, const char* fileName, int32_t line, const char* funcName, const char* tag, const char* format, ...);
		static void clear();
		static std::vector<std::string> copyLogs(int latestItemCount = 0);
		static void swap(std::deque<std::string>& queue);
		static void printLogs(int latestItemCount = 0);
		static void changeLogMaxQueueSize(int newSize);
		static void changeLogLevel(FPLogLevel level);
	};

	#define LOG_INFO(fmt, ...) {FPLog::log(FPLog::FP_LEVEL_INFO,false,__FILE__,__LINE__,__func__, "", fmt, ##__VA_ARGS__);}

	#define LOG_ERROR(fmt, ...) {FPLog::log(FPLog::FP_LEVEL_ERROR,false,__FILE__,__LINE__,__func__, "", fmt, ##__VA_ARGS__);}

	#define LOG_WARN(fmt, ...)  {FPLog::log(FPLog::FP_LEVEL_WARN,false,__FILE__,__LINE__,__func__, "", fmt, ##__VA_ARGS__);}

	#define LOG_DEBUG(fmt, ...) {FPLog::log(FPLog::FP_LEVEL_DEBUG,false,__FILE__,__LINE__,__func__, "", fmt, ##__VA_ARGS__);}

	#define LOG_FATAL(fmt, ...) {FPLog::log(FPLog::FP_LEVEL_FATAL,false,__FILE__,__LINE__,__func__, "", fmt, ##__VA_ARGS__);}

	#define UXLOG(tag, fmt, ...) FPLog::log(FPLog::FP_LEVEL_INFO,true,__FILE__,__LINE__,__func__, tag, fmt, ##__VA_ARGS__);

[FPNN][] 框架的 FPLog 需要配合 LogAgent 服务和 LogServer 服务进行日志的过滤、转发、合并。  
SDK 的 FPLog 本质上为日志缓存模块，需要用户必要时，调用 `swap()` 或者 `printLogs()` 接口输出缓存的日志。无需 LogAgent 服务和 LogServer 服务配合。

**注意：目前 C++ SDK 所有内部日志全部使用 FPLog 记录。**

* **`static FPLogPtr instance()`**

	获取 FPLog 实例对象。

* **`static void log(FPLogLevel curLevel, bool compulsory, const char* fileName, int32_t line, const char* funcName, const char* tag, const char* format, ...)`**

	记录日志。一般不直接使用该接口，而使用 `LOG_XXXX` 系列宏定义。具体请参见后续描述。

* **`static void clear()`**

	清除当前缓存的所有日志。

* **`static std::vector<std::string> copyLogs(int latestItemCount = 0)`**

	拷贝最近 `latestItemCount` 条日志。倒序排列。

* **`static void swap(std::deque<std::string>& queue)`**

	交换日志队列。

* **`static void printLogs(int latestItemCount = 0)`**

	以 `std::cout` 形式输出最近 `latestItemCount` 条日志。

* **`static void changeLogMaxQueueSize(int newSize)`**

	修改日志最大缓存数量限制。

* **`static void changeLogLevel(FPLogLevel level)`**

	修改日志缓存级别。

* **`#define LOG_INFO(fmt, ...)`**

	以 `INFO` 级别记录日志。如果当前日志缓存级别高于 `INFO` 级别，则该日志将被忽略。具体使用方式类似于 `printf` 函数。

* **`#define LOG_ERROR(fmt, ...)`**

	以 `ERROR` 级别记录日志。如果当前日志缓存级别高于 `ERROR` 级别，则该日志将被忽略。具体使用方式类似于 `printf` 函数。

* **`#define LOG_WARN(fmt, ...)`**

	以 `WARN` 级别记录日志。如果当前日志缓存级别高于 `WARN` 级别，则该日志将被忽略。具体使用方式类似于 `printf` 函数。

* **`#define LOG_DEBUG(fmt, ...)`**

	以 `DEBUG` 级别记录日志。如果当前日志缓存级别高于 `DEBUG` 级别，则该日志将被忽略。具体使用方式类似于 `printf` 函数。

* **`#define LOG_FATAL(fmt, ...)`**

	以 `FATAL` 级别记录日志。如果当前日志缓存级别高于 `FATAL` 级别，则该日志将被忽略。具体使用方式类似于 `printf` 函数。

* **`#define UXLOG(tag, fmt, ...)`**

	以 `INFO` 级别记录日志，该日志不受当前日志缓存级别的影响，将被确定记录。使用方式类似于 `printf` 函数。

	`tag` 为 `char *` 类型。
	

### [FpnnError.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/FpnnError.h)

FPNN 内部异常 & 错误码定义。

#### FpnnError

	class FpnnError: public std::exception
	{
	public:
		FpnnError(const char *file, const char* fun, int32_t line, int32_t code = 0, const std::string& msg = "");
		virtual ~FpnnError() noexcept;

		virtual FpnnError* clone() const;
		virtual void do_throw() const;

		virtual const char* what() const noexcept;

		const char* file() const noexcept;
		const char* fun() const noexcept;
		int32_t line() const noexcept;
		int32_t code() const noexcept;
		const std::string& message() const noexcept;

		static std::string format(const char *fmt, ...);
	};

FPNN 异常对象。SDK 中，个别 API 可能会抛出该异常。

#### errorCode

具体错误码可以参见 [errorCode](../errorCode.md)。

### [HashFunctor.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/HashFunctor.h)

Hash 函数对象。具体请参见代码文件。

### [hashint.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/hashint.h)

整型类型 hash 函数。

	uint32_t hash32_uint(unsigned int a);
	uint32_t hash32_ulong(unsigned long a);
	uint32_t hash32_ulonglong(unsigned long long a);
	uint32_t hash32_uintptr(uintptr_t a);

	uint32_t hash32_uint32(uint32_t a);
	uint32_t hash32_uint64(uint64_t a);

	uint64_t hash64_uint64(uint64_t a);

	uint32_t hash32_mix(uint32_t a, uint32_t b, uint32_t c);

### [hex.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/hex.h)

16进制工具。

* **`int hexlify(char *dst, const void *src, int size)`**

	将 `const void *src` 指向的内存中，`int size` 字节的二进制数据，转换成**小写**16进制文本，输出到 `char *dst` 中。  
	返回值为输出的长度，不计入结尾的`\0`。

* **`int Hexlify(char *dst, const void *src, int size)`**

	将 `const void *src` 指向的内存中，`int size` 字节的二进制数据，转换成**大写**16进制文本，输出到 `char *dst` 中。  
	返回值为输出的长度，不计入结尾的`\0`。

* **`int unhexlify(void *dst, const char *src, int size)`**

	将 `const void *src` 指向的 `int size` 字节长16进制文本，转化为二进制数据，并输出到 `void *dst` 所指向内存中。  
	如果 `int size` 参数为 `-1`，则 `unhexlify()` 将调用 `stelen(src)` 计算实际长度。

	返回值为实际写入到 `void *dst` 的字节数。  
	如果发生错误，将返回负值。其绝对值为实际使用的 `const void *src` 的字节数。

### [httpcode.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/httpcode.h)

* **`const char *httpcode_description(int code)`**

	根据 HTTP code 获取描述信息。

### [ignoreSignals.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/ignoreSignals.h)

* **`inline void ignoreSignals()`**

	忽略 `SIGHUP`、`SIGXFSZ`、`SIGPIPE`、`SIGTERM`、`SIGUSR1`、`SIGUSR2` 6个信号引发的中断。

### [ITaskThreadPool.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/ITaskThreadPool.h)

线程池接口定义文件。

因为 C++ SDK 仅提供了一个实现 TaskThreadPool，因此请参阅 “TaskThreadPool” 一节。

### [jenkins.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/jenkins.h)

Jenkins hash 算法。

* **`uint32_t jenkins_hash(const void *key, size_t length, uint32_t initval)`**

	参数：

	+ `const void *key`

		需要 hash 的对象的内存地址。

	+ `size_t length`

		需要 hash 的内存的字节长度。

	+ `uint32_t initval`

		hash 的初始值/上轮 hash 的值。

* **`uint64_t jenkins_hash64(const void *key, size_t length, uint64_t initval)`**

	参数：

	+ `const void *key`

		需要 hash 的对象的内存地址。

	+ `size_t length`

		需要 hash 的内存的字节长度。

	+ `uint64_t initval`

		hash 的初始值/上轮 hash 的值。

* 其余不太常用函数，请参见代码文件。

### [LruHashMap.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/LruHashMap.h)

#### LruHashMap

	template<typename Key, typename Data, typename HashFun=HashFunctor<Key> >
	class LruHashMap
	{
		struct Node
		{
			Key key;
			Data data;
		};

	public:
		typedef Key key_type;
		typedef Data data_type;
		typedef Node node_type;
		typedef HashFun hash_func;

		LruHashMap(size_t slot_num, size_t max_size = 0);
		~LruHashMap();

		size_t count() const;

		node_type* find(const Key& key);
		node_type* use(const Key& key);
		node_type* insert(const Key& key, const Data& val);
		node_type* replace(const Key& key, const Data& val);
		bool remove(const Key& key);
		void remove_node(node_type* node);
		void fresh_node(node_type* node);
		void stale_node(node_type* node);
		size_t drain(size_t num);

		node_type *most_fresh() const;
		node_type *most_stale() const;
		node_type *next_fresh(const node_type *node) const;
		node_type *next_stale(const node_type *node) const;
	};

最近最少使用的 hash 字典容器。

* **`LruHashMap(size_t slot_num, size_t max_size = 0)`**

	初始化函数。

	**参数**

	+ `slot_num`

		hash 槽数。会被圆整为 2 的次方。

	+ `max_size`

		最大 hash 槽数。`0`表示与 `slot_num` 相同。

* **`size_t count() const`**

	当前容器包含条目数量。

* **`node_type* find(const Key& key)`**

	查找对象。但不改变 LRU 状态。

* **`node_type* use(const Key& key)`**

	查找，如果找到，更新 LRU 状态。

* **`node_type* insert(const Key& key, const Data& val)`**

	插入数据。  
	如果键值相同，则取消插入，并返回 `NULL`，否则返回插入后的节点指针。

* **`node_type* replace(const Key& key, const Data& val)`**

	插入或者覆盖数据。  
	返回插入后/覆盖后节点的指针。

* **`bool remove(const Key& key)`**

	删除 `key` 对应的节点 & 数据。

* **`void remove_node(node_type* node)`**

	删除 `node` 对应的节点 & 数据。

* **`void fresh_node(node_type* node)`**

	将节点对应的 LRU 状态调整为最近使用。

* **`void stale_node(node_type* node)`**

	将节点对应的 LRU 状态调整为最久未使用。

* **`size_t drain(size_t num)`**

	删除最久未使用的 `num` 个节点。

* **`node_type *most_fresh() const`**

	返回最近使用的节点。

* **`node_type *most_stale() const`**

	返回最久未使用的节点。

* **`node_type *next_fresh(const node_type *node) const`**

	下一个最近使用的节点。

* **`node_type *next_stale(const node_type *node) const`**

	下一个最久未使用的节点。

### [md5.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/md5.h)

* **md5 上下文结构**

		typedef struct {
			uint32_t total[2];
			uint32_t state[4];
			unsigned char buffer[64];
		} md5_context;

* **`void md5_start(md5_context *ctx)`**

	初始化 md5 上下文结构。

* **`void md5_update(md5_context *ctx, const void *input, size_t length)`**

	使用 `md5_context *ctx` 指定的上下文，继续计算 `const void *input` 指向的 `size_t length` 字节长的数据的 md5。

* **`void md5_finish(md5_context *ctx, unsigned char digest[16])`**

	md5 计算完成，获取 md5 数值。

* **`void md5_checksum(unsigned char digest[16], const void *input, size_t length)`**

	计算 `const void *input` 所指向 `size_t length` 字节长的数据的 md5 值，并在 `unsigned char digest[16]` 中返回。

### [msec.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/msec.h)

与 [FPNN][] 框架兼容的文件。提供 `int64_t slack_real_msec()` 和 `int64_t slack_real_sec()` 两个函数 C++ SDK 的特定实现。

**注意：C++ SDK 的实现与 [FPNN][] 框架的实现完全不同，仅是函数兼容。**

* **`int64_t slack_real_sec()`**

	返回 UTC 秒级时间戳。

* **`int64_t slack_real_msec()`**

	返回 UTC 毫秒级时间戳。

### [NetworkUtility.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/NetworkUtility.h)

常用网络函数库。

* **`bool nonblockedFd(int fd)`**

	将文件描述符设置为非阻塞模式。

* **`std::string IPV4ToString(uint32_t internalAddr)`**

	将二进制表示的 IPv4 地址转换为文本形式。

* **`bool checkIP4(const std::string& ip)`**

	检查是否是有效的 IPv4 地址。

* **EndPointType 枚举**

		enum EndPointType{
			ENDPOINT_TYPE_IP4 = 1,
			ENDPOINT_TYPE_IP6 = 2,
			ENDPOINT_TYPE_DOMAIN = 3,
		};

* **`bool parseAddress(const std::string& address, std::string& host, int& port)`**

	将 endpoint/address 拆解为域名/IP和端口两个部分。

* **`bool parseAddress(const std::string& address, std::string& host, int& port, EndPointType& eType)`**

	将 endpoint/address 拆解为域名/IP和端口两个部分，顺带识别 endpoint 类型。

* **`bool getIPAddress(const std::string& hostname, std::string& IPAddress, EndPointType& eType)`**

	DNS 解析，获取 `const std::string& hostname` 对应的 IP 地址，以及 IP 地址类型。
	如果 `hostname` 对应多个地址，IPv4 类型地址会被优先返回。

* **`bool NetworkUtil::isPrivateIP(struct sockaddr_in* addr)`**

	检查 `struct sockaddr_in* addr` 所表述的 IPv4 地址，是否是本地地址，或者内网地址。

* **`bool NetworkUtil::isPrivateIP(struct sockaddr_in6* addr)`**

	检查 `struct sockaddr_in6* addr` 所表述的 IPv6 地址，是否是本地地址，或者内网地址。

* **`bool NetworkUtil::isPrivateIPv4(const std::string& ipv4)`**

	检查 `const std::string& ipv4` 所表述的 IPv4 地址，是否是本地地址，或者内网地址。

* **`bool NetworkUtil::isPrivateIPv6(const std::string& ipv6)`**

	检查 `const std::string& ipv6` 所表述的 IPv6 地址，是否是本地地址，或者内网地址。


### [rijndael.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/rijndael.h)

AES 加密算法。  
rijndael 为该算法被确定为 AES 加密算法前的名字。

* **rijndael 上下文**

		typedef struct {
			int nrounds;
			uint32_t rk[60];
		} rijndael_context;

* **`bool rijndael_setup_encrypt(rijndael_context *ctx, const uint8_t *key, size_t keylen)`**

	初始化加密上下文。

	`keylen` 必须为 16、24、32 之一。

* **`bool rijndael_setup_decrypt(rijndael_context *ctx, const uint8_t *key, size_t keylen)`**

		初始化解密上下文。

	`keylen` 必须为 16、24、32 之一。

* **`void rijndael_encrypt(const rijndael_context *ctx, const uint8_t plain[16], uint8_t cipher[16])`**

	AES/rijndael 加密函数。`const uint8_t plain[16]` 和 `uint8_t cipher[16]` 可以指向同一内存区域。

* **`void rijndael_decrypt(const rijndael_context *ctx, const uint8_t cipher[16], uint8_t plain[16])`**

	AES/rijndael 解密函数。`const uint8_t cipher[16]` 和 `uint8_t plain[16]` 可以指向同一内存区域。

* **`void rijndael_cbc_encrypt(const rijndael_context *ctx, const uint8_t *plain, uint8_t *cipher, size_t len, uint8_t ivec[16])`**

	AES/rijndael 加密函数，采用 CBC 密码分组模式。

	**注意**

	`len` 为 `uint8_t *plain` 指向数据的字节数。如果 `len` 不为 16 的倍数，那 `int8_t *cipher` 指向的输出数据，将会以 `\0` 进行填充，使输出总字节数为 16 的倍数。

* **`void rijndael_cbc_decrypt(const rijndael_context *ctx, const uint8_t *cipher, uint8_t *plain, size_t len, uint8_t ivec[16])`**

	AES/rijndael 解密函数，采用 CBC 密码分组模式。

	**注意**

	`len` 为 `uint8_t *plain` 指向数据的字节数。如果 `len` 不为 16 的倍数，那多余的数据(加密时填充的数据)将被舍弃。

* **`void rijndael_cfb_encrypt(const rijndael_context *ctx, bool encrypt, const uint8_t *in, uint8_t *out, size_t len, uint8_t ivec[16], size_t *p_num)`**

	AES/rijndael 加解密函数，采用 CFB 密文反馈模式。

	`size_t *p_num` 指向上次加解密的位置偏移。

	**注意**

	+ 不论加密或者解密，`const rijndael_context *ctx` 都的使用 `rijndael_setup_encrypt()` 初始化。

	+ 参数 `bool encrypt` 决定执行加密，还是解密。

* **`void rijndael_ofb_encrypt(const rijndael_context *ctx, const uint8_t *in, uint8_t *out, size_t len, uint8_t ivec[16], size_t *p_num)`**

	AES/rijndael 加解密函数，采用 OFB 输出反馈模式。

	`size_t *p_num` 指向上次加解密的位置偏移。

	**注意**

	+ 不论加密或者解密，`const rijndael_context *ctx` 都的使用 `rijndael_setup_encrypt()` 初始化。

	+ 加解密均使用同一函数。

### [sha1.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/sha1.h)

SHA-1 签名函数。

* **sha-1 上下文结构**

		typedef struct {
			uint32_t total[2];
			uint32_t state[5];
			unsigned char buffer[64];
		} sha1_context;

* **`void sha1_start(sha1_context *ctx)`**

	初始化 sha1 上下文结构。

* **`void sha1_update(sha1_context *ctx, const void *input, size_t length)`**

	使用 `sha1_context *ctx` 指定的上下文，继续计算 `const void *input` 指向的 `size_t length` 字节长的数据的 sha1 值。

* **`void sha1_finish(sha1_context *ctx, unsigned char digest[20])`**

	sha1 计算完成，获取 sha1 数值。

* **`void sha1_checksum(unsigned char digest[20], const void *input, size_t length)`**

	计算 `const void *input` 所指向 `size_t length` 字节长的数据的 sha1 值，并在 `unsigned char digest[20]` 中返回。

### [sha256.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/sha256.h)

SHA-2 签名函数。

* **sha-2 上下文结构**

		typedef struct {
			uint32_t total[2];
			uint32_t state[8];
			unsigned char buffer[64];
		} sha256_context;

* **`void sha256_start(sha256_context *ctx)`**

	初始化 sha2 上下文结构。

* **`void sha256_update(sha256_context *ctx, const void *input, size_t length)`**

	使用 `sha256_context *ctx` 指定的上下文，继续计算 `const void *input` 指向的 `size_t length` 字节长的数据的 sha2 值。

* **`void sha256_finish(sha256_context *ctx, unsigned char digest[32])`**

	sha2 计算完成，获取 sha2 数值。

* **`void sha256_checksum(unsigned char digest[32], const void *input, size_t length)`**

	计算 `const void *input` 所指向 `size_t length` 字节长的数据的 sha2 值，并在 `unsigned char digest[32]` 中返回。

### [StringUtil.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/StringUtil.h)

字符串处理工具模块。

**注意：本模块所有函数和对象，均处于 `StringUtil` 子命令空间内。**

#### 函数

* **trim 系列**

		char * rtrim(char *s);

	裁剪右侧。无数据移动，但**可能会修改原始数据**。

		char * ltrim(char *s);

	裁剪左侧。无数据移动，返回新的起始指针。

		char * trim(char *s);

	双端裁剪。无数据移动，但**可能会修改原始数据**，并返回新的起始指针。

		std::string& rtrim(std::string& s);

	裁剪右侧，**会修改原始数据**。

		std::string& ltrim(std::string& s);

	裁剪左侧，**会修改原始数据**。

		std::string& trim(std::string& s);

	双端裁剪。**会修改原始数据**。

		void softTrim(const char* path, char* &start, char* &end);

	双端裁剪，无数据移动，**不修改**原始数据。返回有效数据的起始和结束指针。**注意**，有效数据不包含 `end` 指向位置的字符。

* **replace**

		bool replace(std::string& str, const std::string& from, const std::string& to);

	将 `str` 中 `from` 第一次出现的地方替换为 `to`。

* **split 系列**

		//will discard empty field
		std::vector<std::string> &split(const std::string &s, const std::string& delim, std::vector<std::string> &elems);

		//-- Only for integer or long.
		template<typename T>
		std::vector<T> &split(const std::string &s, const std::string& delim, std::vector<T> &elems);

		//-- Only for integer or long.
		template<typename T>
		std::set<T> &split(const std::string &s, const std::string& delim, std::set<T> &elems);

		//-- Only for integer or long.
		template<typename T>
		std::unordered_set<T> &split(const std::string &s, const std::string& delim, std::unordered_set<T> &elems);

		std::set<std::string> &split(const std::string &s, const std::string& delim, std::set<std::string> &elems);
		std::unordered_set<std::string> &split(const std::string &s, const std::string& delim, std::unordered_set<std::string> &elems);

	按照 `delim` 中出现的**字符**切分字符串 `s`，忽略空串后，放入容器对象 `elems` 中，并将 `elems` 返回。

	**注意：**其中模版函数仅适用于**整型**类型。

* **join 系列**

		template<typename T>
		std::string join(const std::set<T> &v, const std::string& delim);

		template<typename T>
		std::string join(const std::vector<T> &v, const std::string& delim);

		std::string join(const std::set<std::string> &v, const std::string& delim);
		std::string join(const std::vector<std::string> &v, const std::string& delim);
		std::string join(const std::map<std::string, std::string> &v, const std::string& delim);

	将容器 `v` 中的数据，用字符串 `delim` 拼接并返回。

#### CharsChecker

字符匹配检查类。

	class CharsChecker
	{
	public:
		CharsChecker(const unsigned char *targets);
		CharsChecker(const unsigned char *targets, int len);
		CharsChecker(const std::string& targets);
		inline bool operator[] (unsigned char c);
	};

主要用于自定义字符集合的检查。类似于 `isspace()` 等函数。

#### CharMarkMap

字符位掩码匹配检查。

	template<typename UNSIGNED_INTEGER_TYPE>
	class CharMarkMap
	{
	public:
		CharMarkMap();
		void init(const unsigned char *targets, UNSIGNED_INTEGER_TYPE mark);
		void init(const unsigned char *targets, int len, UNSIGNED_INTEGER_TYPE mark);

		void init(unsigned char c, UNSIGNED_INTEGER_TYPE mark);

		inline void init(const char *targets, UNSIGNED_INTEGER_TYPE mark);
		inline void init(const char *targets, int len, UNSIGNED_INTEGER_TYPE mark);
		inline void init(char c, UNSIGNED_INTEGER_TYPE mark);

		inline UNSIGNED_INTEGER_TYPE operator[] (unsigned char c);

		inline bool check(unsigned char c, UNSIGNED_INTEGER_TYPE mark);

		inline bool check(char c, UNSIGNED_INTEGER_TYPE mark);
	};

主要用于自定义的字符集合及其成员字符关联掩码的匹配和查询。

### [TaskThreadPool.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/TaskThreadPool.h)

C++ SDK 默认线程池。

#### ITask

	class ITaskThreadPool
	{
	public:
		class ITask
		{
			public:
				virtual void run() = 0;
				virtual ~ITask() {}
		};
		typedef std::shared_ptr<ITask> ITaskPtr;
	};

线程池任务接口定义。

#### FunctionTask

	class TaskThreadPool: public ITaskThreadPool
	{
	public:
		class FunctionTask: public ITask
		{
			public:
				explicit FunctionTask(std::function<void ()> function);
				virtual ~FunctionTask();
				virtual void run();
		};
	};

线程池任务接口对 lambda 的实现。

#### TaskThreadPool

	class TaskThreadPool: public ITaskThreadPool
	{
	public:
		virtual bool			init(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueLength = 0, size_t tempThreadLatencySeconds = 60);
		virtual bool			wakeUp(ITaskPtr task);
		virtual bool			wakeUp(std::function<void ()> task);
		
		virtual void			release();
		virtual void			status(int32_t &normalThreadCount, int32_t &temporaryThreadCount, int32_t &busyThreadCount, int32_t &taskQueueSize, int32_t& min, int32_t& max, int32_t& maxQueue);
		virtual std::string		infos();
		virtual bool inited();
		virtual bool exiting();
		TaskThreadPool();
		virtual ~TaskThreadPool();
	};

* **`virtual bool init(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount, size_t maxQueueLength = 0, size_t tempThreadLatencySeconds = 60)`**

	线程池初始化函数。

	**参数**

	+ **int32_t initCount**

		初始的线程数量。

	+ **int32_t perAppendCount**

		追加线程时，单次的追加线程数量。

		**注意**，当线程数超过 `perfectCount` 的限制后，单次追加数量仅为 `1`。

	+ **int32_t perfectCount**

		线程池常驻线程的最大数量。

	+ **int32_t maxCount**

		线程池线程最大数量。如果为 `0`，则表示不限制。

	+ **size_t maxQueueLength**

		线程池任务队列最大数量限制。如果为 `0`，则表示不限制。

	+ **size_t tempThreadLatencySeconds**

		超过 `perfectCount` 数量限制的临时线程退出前，等待新任务的等待时间。

* **`virtual bool wakeUp(ITaskPtr task)`**

	执行任务。

* **`virtual bool wakeUp(std::function<void ()> task)`**

	执行任务。

* **`virtual void release()`**

	释放线程池。  
	释放线程池会等待所有线程将任务队列执行完毕。

	释放后的线程池，可重新初始化。

* **`virtual void status(int32_t &normalThreadCount, int32_t &temporaryThreadCount, int32_t &busyThreadCount, int32_t &taskQueueSize, int32_t& min, int32_t& max, int32_t& maxQueue)`**

	获取线程池状态。

	**参数**

	+ **`normalThreadCount`**

		当前常驻线程数量。

	+ **`temporaryThreadCount`**

		当前临时线程数量。

	+ **`busyThreadCount`**

		当前正在执行任务的线程数量。

	+ **`taskQueueSize`**

		当前待处理的任务数量（不含执行中的）。

	+ **`min`**

		初始化线程池时的初始化线程数。

	+ **`max`**

		最大线程数限制。

	+ **`maxQueue`**

		最大任务队列限制。


* **`virtual std::string infos()`**

	以 JSON 格式，返回线程池状态。

* **`virtual bool inited()`**

	判断线程池是否初始化。

* **`virtual bool exiting()`**

	判断线程池是否正在释放/退出。


### [TimeUtil.h](https://github.com/highras/fpnn-sdk-cpp/blob/master/src/base/TimeUtil.h)

时间格式化工具函数，和时间相关函数。

**注意：本模块所有函数和对象，均处于 `TimeUtil` 子命令空间内。**

* **`std::string getTimeRFC1123()`**

	按 RFC1123 的格式，格式化当前 UTC 时间。

* **`std::string getDateTime()`**

	按 `yyyy-MM-dd HH:mm:ss` 格式，格式化当前时间。

* **`std::string getDateTime(int64_t t);`**

	按 `yyyy-MM-dd HH:mm:ss` 格式，格式化 `t` 指定的时间。

* **`std::string getDateStr(char sep = '-')`**

	按 `yyyy-MM-dd` 格式，格式化当前时间。连接字符由 `sep` 参数指定。

* **`std::string getDateStr(int64_t t, char sep = '-')`**

	按 `yyyy-MM-dd` 格式，格式化 `t` 指定的时间。连接字符由 `sep` 参数指定。

* **`std::string getDateTimeMS()`**

	按 `yyyy-MM-dd HH:mm:ss,SSS` 格式，格式化当前时间。

* **`std::string getDateTimeMS(int64_t t)`**

	按 `yyyy-MM-dd HH:mm:ss,SSS` 格式，格式化 `t` 指定的时间。

* **`int64_t curr_sec()`**

	返回当前 UTC 秒级时间戳。  
	**注意：**请避免直接使用该函数，请使用 `msec.h` 中的兼容包装 `slack_real_sec()` 作为替代。

* **`int64_t curr_msec()`**

	返回当前 UTC 毫秒级时间戳。  
	**注意：**请避免直接使用该函数，请使用 `msec.h` 中的兼容包装 `slack_real_msec()` 作为替代。


[FPNN]: https://github.com/highras/fpnn


