#ifndef C_LOG_CORE_H_
#define C_LOG_CORE_H_

#include "../Logger/AsyncLogger.h"

namespace AsyncLogSDK
{
	#define LOGINFO(...) CAsyncLogger::getInstance()->writeLog(EI_LOG_INFO, __FUNCSIG__, __LINE__, __VA_ARGS__);
	#define LOGERROR(...) CAsyncLogger::getInstance()->writeLog(EI_LOG_ERROR, __FUNCSIG__, __LINE__, __VA_ARGS__);
	#define LOGWARNING(...) CAsyncLogger::getInstance()->writeLog(EI_LOG_WARNING, __FUNCSIG__, __LINE__, __VA_ARGS__);
	#define LOGFATAL(...) CAsyncLogger::getInstance()->writeLog(EI_LOG_FATAL, __FUNCSIG__, __LINE__, __VA_ARGS__);
}

#endif //!C_LOG_CORE_H_