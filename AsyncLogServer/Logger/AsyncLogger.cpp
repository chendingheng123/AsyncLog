#include "AsyncLogger.h"
#include "AsyncLoggerImpl.h"

using namespace AsyncLogSDK;

CAsyncLogger* CAsyncLogger::mAsyncLogger = nullptr;
CAsyncLogger* CAsyncLogger::getInstance()
{
	if (mAsyncLogger == nullptr)
		mAsyncLogger = new CAsyncLogger;
	return mAsyncLogger;
}

bool CAsyncLogger::init()
{
	std::unique_lock<std::mutex> lk(initMutex);
	if (mbInited)
		return mbInited;

	if (asyncLogImplPtr)
		mbInited = asyncLogImplPtr->initLogger();

	return mbInited;
}

CAsyncLogger::CAsyncLogger()
{
	asyncLogImplPtr = new CAsyncLoggerImpl;
}

CAsyncLogger::~CAsyncLogger()
{
	//防止外部多个业务逻辑同时主动释放CAsyncLogger引发线程安全问题，否则单例的生命周期在dll被卸载时才结束。
	std::unique_lock<std::mutex> lk(initMutex);
	if (asyncLogImplPtr)
	{
		asyncLogImplPtr->uninitLogger();
		mbInited = false;
		delete asyncLogImplPtr;
		asyncLogImplPtr = nullptr;
	}
}

void CAsyncLogger::writeLog(LogLevel logLevel, PCSTR functionName, int lineNo, PCSTR sContentFmt, ...)
{
	//拼上日志正文
	std::string sLogMsg;
	va_list ap;
	char logBuf[1024] = { 0 };
	va_start(ap, sContentFmt);
#if defined(_WIN32) || defined(_WIN64)
	int iLogMsgLength = _vscprintf(sContentFmt, ap);
	int iLogCapactity = static_cast<int>(sLogMsg.capacity());
	if (iLogCapactity < iLogMsgLength + 1)
		sLogMsg.resize((size_t)iLogMsgLength + 1);
	vsprintf_s((char*)sLogMsg.c_str(), (size_t)iLogMsgLength + 1, sContentFmt, ap);
#elif defined(__linux__)
	int offset = snprintf(logBuf, sizeof(logBuf), "%d,%s,%d", logLevel, functionName, lineNo);
	vsnprintf(logBuf + offset, sizeof(logBuf) - offset, sContentFmt, ap);
#endif
	va_end(ap);
#if defined(__linux__)
	char* pBuf = logBuf;
	pBuf = pBuf + offset;
	sLogMsg = pBuf;
#elif defined(_WIN32) || defined(_WIN64)
	sLogMsg = sLogMsg.substr(0, sLogMsg.length() - 1);
#endif

	std::string sFormatMsg = "[" + sLogMsg + "]";
	if (asyncLogImplPtr)
		return asyncLogImplPtr->writeLog(logLevel, functionName, lineNo, sFormatMsg);
}

void CAsyncLogger::getLogList(std::list<LogDataBlock>& cachedLogList)
{
	std::unique_lock<std::mutex> lk(initMutex);
	if (asyncLogImplPtr)
		return asyncLogImplPtr->getCacheLogList(cachedLogList);
}

void CAsyncLogger::getFileList(std::vector<FileInfo>& fileInfoList)
{
	std::unique_lock<std::mutex> lk(initMutex);
	if (asyncLogImplPtr)
		asyncLogImplPtr->getCacheFileList(fileInfoList);
}