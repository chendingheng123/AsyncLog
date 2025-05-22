#ifndef C_ASYNC_LOG_IMPL_H_
#define C_ASYNC_LOG_IMPL_H_

#include "../LogCore/Global.h"
#include "LogView/LogPreview.h"
#include <mutex>
#include <semaphore>

namespace AsyncLogSDK
{
	class CAsyncLoggerImpl
	{	
		public:
			CAsyncLoggerImpl();
			~CAsyncLoggerImpl();

			bool initLogger();
			void uninitLogger();
			ErrorCode writeFile(const std::string& sContent);
			void writeLog(LogLevel logLevel, PCSTR functionName, int lineNo, const std::string& sFormatMsg);
			void getCacheLogList(std::list<LogDataBlock>& cachedLogList);
			void getCacheFileList(std::vector<FileInfo>& fileInfoList);

		protected:
			void logConsumeThread();
			void logBakThread();
			void createNewLogFile();

		private:
			std::deque<LogDataBlock> mCachedLogList;
			std::deque<LogDataBlock> mSortedLogList;
			std::deque<LogDataBlock> mReadyLogList;
			HANDLE mExitSem = INVALID_HANDLE_VALUE;
			HANDLE mBakEvent = INVALID_HANDLE_VALUE;
			HANDLE mFile = INVALID_HANDLE_VALUE;
			std::mutex mLogMtx;
			std::condition_variable mConsumeCon;
			std::unique_ptr<std::thread> mBakThread;
			std::vector<std::unique_ptr<std::thread>> mConsumLogThreadVec;
			int mConsumeThreadCnt = MAX_LOG_CONSUME_NUM;
			int mWriteIndex = 0;
	};
}

#endif // !C_ASYNC_LOG_IMPL_H_