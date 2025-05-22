#ifndef C_GLOBAL_H_
#define C_GLOBAL_H_

#include <algorithm>
#include <chrono>
#include <ctime>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <Windows.h>

#define AsyncLogApi __declspec(dllexport)

namespace AsyncLogSDK
{
	#define MAX_LOG_CONSUME_NUM 2
	#define MAX_LOG_PRODUCE_NUM 100
	#define MAX_LOG_FILE_NUM 10
	#define MAX_LOG_FILE_SIZE 10*1024

	enum LogLevel
	{
		EI_LOG_NONE = 0,
		EI_LOG_INFO,
		EI_LOG_WARNING,
		EI_LOG_ERROR,
		EI_LOG_FATAL
	};

	enum ErrorCode
	{
		EI_NO_ERROR,
		EI_FULL_SIZE,
		EI_WRITE_ERROR
	};

	enum QueryStatus
	{
		EI_NONE,
		EI_PROCESSING,
		EI_COMPLETE
	};

	struct LogQueryStu
	{
	public:
		LogQueryStu()
		{
			logLevel = EI_LOG_NONE;
			logStartTime = { 0 };
			logEndTime = { 0 };
		}

		bool empty()
		{
			return logLevel == EI_LOG_NONE
				&& function.empty()
				&& logStartTime.dwHighDateTime == 0 && logStartTime.dwLowDateTime == 0
				&& logEndTime.dwHighDateTime == 0 && logEndTime.dwLowDateTime == 0;
		}

		bool timeValid()
		{
			return CompareFileTime(&logStartTime, &logEndTime) < 0;
		}

		bool isStartTimeValid()
		{
			return logStartTime.dwHighDateTime > 0 && logStartTime.dwLowDateTime > 0;
		}

		bool isEndTimeValid()
		{
			return logEndTime.dwHighDateTime > 0 && logEndTime.dwLowDateTime > 0;
		}

	
	 public:
		std::string function;
		FILETIME logStartTime;
		FILETIME logEndTime;
		LogLevel logLevel;
	};

	struct LogDataBlock
	{
	  public:
		LogDataBlock()
		{
			logTime = { 0 };
			logLevel = EI_LOG_INFO;
		}

		bool operator < (const LogDataBlock& log)
		{
			return CompareFileTime(&this->logTime, &log.logTime);
		}

	 public:
		FILETIME logTime;
		std::string logData;
		std::string logFunction;
		LogLevel logLevel;
	};

	struct FileInfo
	{
	  public:
		  FileInfo()
		  {
			  fileTime = { 0 };
		  }

	 public:
		std::wstring filePath;
		FILETIME fileTime;
	};

	std::string wideCharConvertMultiByte(LPCWSTR lpcwStr);
	std::wstring getCurrentModulePath();
	void searchFilesInDir(LPCWSTR sDirPath, std::vector<FileInfo>& fileInfoList);
}

#endif //!C_GLOBAL_H_

