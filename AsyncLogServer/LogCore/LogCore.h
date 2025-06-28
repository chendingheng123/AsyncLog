#ifndef C_LOG_CORE_H_
#define C_LOG_CORE_H_

#include "../Logger/AsyncLogger.h"

namespace AsyncLogSDK
{
#if defined(_WIN32) || (_WIN64)
#define LOGINFO(...) CAsyncLogger::getInstance()->writeLog(EI_LOG_INFO, __FUNCSIG__, __LINE__, __VA_ARGS__);
#define LOGERROR(...) CAsyncLogger::getInstance()->writeLog(EI_LOG_ERROR, __FUNCSIG__, __LINE__, __VA_ARGS__);
#define LOGWARNING(...) CAsyncLogger::getInstance()->writeLog(EI_LOG_WARNING, __FUNCSIG__, __LINE__, __VA_ARGS__);
#define LOGFATAL(...) CAsyncLogger::getInstance()->writeLog(EI_LOG_FATAL, __FUNCSIG__, __LINE__, __VA_ARGS__);
#elif defined(__linux__)
#define LOGINFO(sContentFmt, ...) CAsyncLogger::getInstance()->writeLog(EI_LOG_INFO, __PRETTY_FUNCTION__, __LINE__, sContentFmt, ##__VA_ARGS__);
#define LOGERROR(sContentFmt, ...) CAsyncLogger::getInstance()->writeLog(EI_LOG_ERROR, __PRETTY_FUNCTION__, __LINE__, sContentFmt, ##__VA_ARGS__);
#define LOGWARNING(sContentFmt, ...) CAsyncLogger::getInstance()->writeLog(EI_LOG_WARNING, __PRETTY_FUNCTION__, __LINE__, sContentFmt, ##__VA_ARGS__);
#define LOGFATAL(sContentFmt, ...) CAsyncLogger::getInstance()->writeLog(EI_LOG_FATAL, __PRETTY_FUNCTION__, __LINE__, sContentFmt, ##__VA_ARGS__);
#endif	
}

#endif //!C_LOG_CORE_H_