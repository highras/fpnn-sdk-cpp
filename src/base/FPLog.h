#ifndef FPNN_CPP_SDK_LOG_H
#define FPNN_CPP_SDK_LOG_H

#include <atomic>
#include <string>
#include <deque>
#include <vector>
#include <memory>
#include <stdarg.h>

namespace fpnn
{

class FPLog;
typedef std::shared_ptr<FPLog> FPLogPtr;

class FPLog
{
public:
	typedef enum {FP_LEVEL_FATAL=0, FP_LEVEL_ERROR=1, FP_LEVEL_WARN=2, FP_LEVEL_INFO=3, FP_LEVEL_DEBUG=4} FPLogLevel;

private:
	FPLogLevel _level;
	std::atomic<int> _maxQueueSize;
	std::deque<std::string> _logQueue;

	FPLog(): _level(FP_LEVEL_ERROR), _maxQueueSize() {}

public:
	~FPLog();
	
	static FPLogPtr instance();
	static void log(FPLogLevel curLevel, bool compulsory, const char* fileName, int32_t line, const char* funcName, const char* tag, const char* format, ...);
	static void clear();
	static std::vector<std::string> copyLogs(int latestItemCount = 0);		//-- copy latest logs. First log is the latest.
	static void swap(std::deque<std::string>& queue);
	static void printLogs(int latestItemCount = 0);		//-- print latest logs.
	static void changeLogMaxQueueSize(int newSize);
	static void changeLogLevel(FPLogLevel level);
};

#define LOG_INFO(fmt, ...) {FPLog::log(FPLog::FP_LEVEL_INFO,false,__FILE__,__LINE__,__func__, "", fmt, ##__VA_ARGS__);}

#define LOG_ERROR(fmt, ...) {FPLog::log(FPLog::FP_LEVEL_ERROR,false,__FILE__,__LINE__,__func__, "", fmt, ##__VA_ARGS__);}

#define LOG_WARN(fmt, ...)  {FPLog::log(FPLog::FP_LEVEL_WARN,false,__FILE__,__LINE__,__func__, "", fmt, ##__VA_ARGS__);}

#define LOG_DEBUG(fmt, ...) {FPLog::log(FPLog::FP_LEVEL_DEBUG,false,__FILE__,__LINE__,__func__, "", fmt, ##__VA_ARGS__);}

#define LOG_FATAL(fmt, ...) {FPLog::log(FPLog::FP_LEVEL_FATAL,false,__FILE__,__LINE__,__func__, "", fmt, ##__VA_ARGS__);}

#define UXLOG(tag, fmt, ...) FPLog::log(FPLog::FP_LEVEL_INFO,true,__FILE__,__LINE__,__func__, tag, fmt, ##__VA_ARGS__);
}

#endif
