#include <mutex>
#include <iostream>
#include "TimeUtil.h"
#include "FPLog.h"

using namespace fpnn;

static std::mutex gc_FPLogMutex;
static std::atomic<bool> _created(false);
static FPLogPtr _fpLogger;

FPLogPtr FPLog::instance()
{
	if (!_created)
	{
		std::unique_lock<std::mutex> lck(gc_FPLogMutex);
		if (!_created)
		{
			_fpLogger.reset(new FPLog);
			_created = true;
		}
	}
	return _fpLogger;
}

void FPLog::log(FPLogLevel currLevel,bool compulsory, const char* fileName, int32_t line, const char* funcName, const char* tag, const char* format, ...)
{
	const int FPLogBufferSize = 4096;
	static const char* dbgLevelStr[]={"FATAL","ERROR","WARN","INFO","DEBUG"};

	FPLogPtr logger = instance();
	if (!compulsory && (currLevel > logger->_level))
		return;

	char *logItemBuffer = (char*)malloc(FPLogBufferSize);
	if(!logItemBuffer)
		return;

	int32_t s = snprintf(logItemBuffer, FPLogBufferSize, "[%s]~[%s]~[%s@%s:%d]~[%s]: ", 
		TimeUtil::getDateTimeMS().c_str(),
		dbgLevelStr[currLevel], funcName, fileName, line, tag);

	if (s > 0 && s < FPLogBufferSize)
	{
		va_list va;
		va_start(va, format);
		int32_t vs = vsnprintf(logItemBuffer + s, FPLogBufferSize - s, format, va);
		va_end(va);
		if (vs > 0)
		{
			std::unique_lock<std::mutex> lck(gc_FPLogMutex);
			logger->_logQueue.push_back(logItemBuffer);
		}
	}
	free(logItemBuffer);
}

void FPLog::clear()
{
	std::unique_lock<std::mutex> lck(gc_FPLogMutex);
	if (_fpLogger)
		_fpLogger->_logQueue.clear();
}

std::vector<std::string> FPLog::copyLogs(int latestItemCount)
{
	std::vector<std::string> rev;

	std::unique_lock<std::mutex> lck(gc_FPLogMutex);
	if (_fpLogger)
	{
		int count = (int)(_fpLogger->_logQueue.size());

		if (latestItemCount <= 0 && latestItemCount > count)
			latestItemCount = count;

		rev.reserve(latestItemCount);

		for (auto it = _fpLogger->_logQueue.rbegin();
			latestItemCount > 0; it++, latestItemCount--)
		{
			rev.push_back(*it);
		}
	}

	return rev;
}

void FPLog::swap(std::deque<std::string>& queue)
{
	std::unique_lock<std::mutex> lck(gc_FPLogMutex);
	if (_fpLogger)
		_fpLogger->_logQueue.swap(queue);
}

void FPLog::printLogs(int latestItemCount)
{
	std::unique_lock<std::mutex> lck(gc_FPLogMutex);
	if (_fpLogger)
	{
		int count = (int)(_fpLogger->_logQueue.size());

		if (latestItemCount <= 0 || latestItemCount > count)
			latestItemCount = count;

		for (auto it = _fpLogger->_logQueue.begin();
			latestItemCount > 0; it++, latestItemCount--)
		{
			std::cout<<(*it)<<std::endl;
		}
	}
}
void FPLog::changeLogMaxQueueSize(int newSize)
{
	instance()->_maxQueueSize = newSize;
}
void FPLog::changeLogLevel(FPLogLevel level)
{
	instance()->_level = level;
}
