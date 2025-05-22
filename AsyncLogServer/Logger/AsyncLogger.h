#ifndef C_ASYNC_LOG_H_
#define C_ASYNC_LOG_H_

#include "AsyncLoggerImpl.h"
#include "LogView/LogPreview.h"

namespace AsyncLogSDK
{
	class AsyncLogApi CAsyncLogger
	{	
		friend class CLogPreview;
		static CAsyncLogger* mAsyncLogger;
		public:
			CAsyncLogger();
			~CAsyncLogger();
			static CAsyncLogger* getInstance();
			bool init();
			void writeLog(LogLevel logLevel, PCSTR functionName, int lineNo, PCSTR sContentFmt, ...);

		protected:
			void getLogList(std::list<LogDataBlock>& cachedLogList);
			void getFileList(std::vector<FileInfo>& fileInfoList);

		private:
			CAsyncLoggerImpl* asyncLogImplPtr = nullptr;//采用PIMPL的设计思想
			std::mutex initMutex;
			bool mbInited = false;
	}; 
}

#endif // !C_ASYNC_LOG_H_