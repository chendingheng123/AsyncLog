#ifndef C_GLOBAL_H_
#define C_GLOBAL_H_

#include <iostream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <deque>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <string.h>
#include <condition_variable>

#if defined(__linux__)
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>
#include <clocale>
#include <dirent.h>
#include <cstdarg>
#include <cstdio>
#include <fstream>
typedef const wchar_t* LPCWSTR; // Linux/MacOS¼æÈÝ
#define AsyncLogApi __attribute__((visibility("default"))) // Linux SOµ¼³ö
#elif defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#define AsyncLogApi __declspec(dllexport)	
#endif


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
#if defined(__linux__)
			logStartTime = { 0.0 };
			logEndTime = { 0.0 };
#endif
		}

		bool empty()
		{
#if defined(_WIN32) || defined(_WIN64)
			return logLevel == EI_LOG_NONE
				&& function.empty()
				&& logStartTime.dwHighDateTime == 0 && logStartTime.dwLowDateTime == 0
				&& logEndTime.dwHighDateTime == 0 && logEndTime.dwLowDateTime == 0;
#elif defined(__linux__)
			return logLevel == EI_LOG_NONE && function.empty()
				&& logStartTime == 0 && logEndTime == 0;
#endif
		}

		bool timeValid()
		{
#if defined(_WIN32) || defined(_WIN64)
			return CompareFileTime(&logStartTime, &logEndTime) < 0;
#elif defined(__linux__)
			return logStartTime < logEndTime;
#endif
		}

		bool isStartTimeValid()
		{
#if defined(_WIN32) || defined(_WIN64)
			return logStartTime.dwHighDateTime > 0 && logStartTime.dwLowDateTime > 0;
#elif defined(__linux__)
			return logStartTime > 0;
#endif
		}

		bool isEndTimeValid()
		{
#if defined(_WIN32) || defined(_WIN64)
			return logEndTime.dwHighDateTime > 0 && logEndTime.dwLowDateTime > 0;
#elif defined(__linux__)
			return logEndTime > 0;
#endif
		}

	public:
		std::string function;
#if defined(_WIN32) || defined(_WIN64)
		FILETIME logStartTime;
		FILETIME logEndTime;
#elif defined(__linux__)
		double logStartTime;
		double logEndTime;
#endif
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
#if defined(_WIN32) || defined(_WIN64)
			return CompareFileTime(&this->logTime, &log.logTime);
#elif defined(__linux__)
			return this->logTime < log.logTime;
#endif
		}

	public:
#if defined(_WIN32) || defined(_WIN64)
		FILETIME logTime;
#elif defined(__linux__)
		double logTime;
#endif
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
#if defined(_WIN32) || defined(_WIN64)
		std::wstring filePath;
		FILETIME fileTime;
#elif defined(__linux__)
		std::string filePath;
		time_t fileTime;
#endif
	};

	std::string wideCharConvertMultiByte(LPCWSTR lpcwStr);
	std::wstring multiByteConvertWideChar(const std::string& str);
	std::wstring getCurrentModulePath();
	void searchFilesInDir(LPCWSTR sDirPath, std::vector<FileInfo>& fileInfoList);
	bool directionaryExist(const std::string& dirPath);
}

#endif //!C_GLOBAL_H_