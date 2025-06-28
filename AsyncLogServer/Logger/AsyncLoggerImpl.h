#ifndef C_ASYNC_LOG_IMPL_H_
#define C_ASYNC_LOG_IMPL_H_
#include <atomic>
#include "../LogCore/Global.h"
#include "../LogView/LogPreview.h"
#include <mutex>
#if defined(_WIN32) || defined(_WIN64)
#include <semaphore>
#elif defined(__linux__)
#include <fstream>
#include <semaphore.h>
#include <iomanip>
typedef const char* PCSTR;
#endif

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
		bool getCacheFileList(std::vector<FileInfo>& fileInfoList);

	protected:
		void logConsumeThread();
		void logBakThread();
		bool createNewLogFile();

	private:
		std::deque<LogDataBlock> mCachedLogList;
		std::deque<LogDataBlock> mSortedLogList;
		std::condition_variable mConsumeCon;
		std::mutex mFileHandleMtx;
		std::mutex mLogMtx;
		std::mutex mLogPathMtx;
#if defined(_WIN32) || defined(_WIN64)
		HANDLE mExitSem = INVALID_HANDLE_VALUE;
		HANDLE mBakEvent = INVALID_HANDLE_VALUE;
		HANDLE mFile = INVALID_HANDLE_VALUE;
#elif defined(__linux__)
		sem_t mExitSem;
		sem_t mBakEvent;
		std::ofstream mFile;
		std::vector<std::string> mLogPathArr;
#endif
		std::unique_ptr<std::thread> mBakThread;
		std::vector<std::unique_ptr<std::thread>> mConsumLogThreadVec;
		int mConsumeThreadCnt = MAX_LOG_CONSUME_NUM;
		int mWriteIndex = 0;
		std::atomic<bool> mbCreateNewFile{ true };
	};
}

#endif // !C_ASYNC_LOG_IMPL_H_