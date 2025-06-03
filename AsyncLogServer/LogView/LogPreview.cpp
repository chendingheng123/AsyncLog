#include "../Logger/AsyncLogger.h"
#include <fstream>
#include <iterator>
#include "../LogTool/LogAlgorithm.h"
#include "LogPreview.h"

using namespace AsyncLogSDK;

CLogPreview::CLogPreview()
{
}

CLogPreview::~CLogPreview()
{
	if (spQueryThread->joinable())
		spQueryThread->join();
}

void CLogPreview::queryLog(LogQueryStu& queryStu, LogViewCallBack& logViewCB)
{
	spQueryThread.reset(new std::thread(std::bind(&CLogPreview::queryLogThread, this, queryStu, logViewCB)));
}

void CLogPreview::queryLogThread(LogQueryStu& queryStu, LogViewCallBack& logViewCB)
{
	//优先查询内存热数据
	std::list<LogDataBlock> cachedLogList;
	CAsyncLogger::getInstance()->getLogList(cachedLogList);

	//查询磁盘的冷数据
	std::vector<FileInfo> fileInfoList;
	std::string lineData;
	CAsyncLogger::getInstance()->getFileList(fileInfoList);
	std::list<LogDataBlock> diskLogList;
	for (const auto& fileInfo : fileInfoList)
	{
		std::string filePath = wideCharConvertMultiByte(fileInfo.filePath.c_str());
		std::ifstream hFileHandle(filePath.c_str(), std::ios::binary);
		if (!hFileHandle)
			return;

		while (std::getline(hFileHandle, lineData))
		{
			//从每一行数据中提取出重要部分，组装成一个LogDataBlock
			size_t leftBracket = 0;
			int iLoopTime = 0;
			LogDataBlock logData;
			logData.logData = lineData;
			while ((leftBracket = lineData.find("[", 0)) >= 0)
			{
				++iLoopTime;
				size_t rightBracket = lineData.find("]", 0);
				std::string slog = lineData.substr(leftBracket + 1, rightBracket - 1);
				if (iLoopTime == 1)
				{
					SYSTEMTIME st;
					std::istringstream ss(slog);
					char dash, colon;
					ss >> st.wYear >> dash >> st.wMonth >> dash >> st.wDay
						>> st.wHour >> colon >> st.wMinute >> colon >>
						st.wSecond >> colon >> st.wMilliseconds;

					SystemTimeToFileTime(&st, &logData.logTime);
				}
				else if (iLoopTime == 2)
				{
					if (slog == "Info")
						logData.logLevel = EI_LOG_INFO;
					else if (slog == "Error")
						logData.logLevel = EI_LOG_ERROR;
					else if (slog == "Fatal")
						logData.logLevel = EI_LOG_FATAL;
					else if (slog == "Warning")
						logData.logLevel = EI_LOG_WARNING;
				}
				else if (iLoopTime == 3)
				{
					logData.logFunction = slog;
				}
				else
				{
					break;
				}
				lineData = lineData.substr(rightBracket + 1);
			}
			diskLogList.push_back(logData);
		}
	}

	diskLogList.splice(diskLogList.end(), cachedLogList);
	if (queryStu.empty()) //全部查询
	{
		for (const auto& log : diskLogList)
			logViewCB(EI_PROCESSING,log.logData);
		logViewCB(EI_COMPLETE, "");
		return;
	}

	//根据条件筛选出各类数据
	for (const auto& log : diskLogList)
	{
		//查询的结果是7种组合
		if (queryStu.logLevel != EI_LOG_NONE && !queryStu.function.empty() && queryStu.timeValid())
		{
			if (log.logLevel == queryStu.logLevel
				&& stringContain(log.logFunction, queryStu.function) >= 0
				&& CompareFileTime(&log.logTime, &queryStu.logStartTime) >= 0
				&& CompareFileTime(&log.logTime, &queryStu.logEndTime) <= 0)
				logViewCB(EI_PROCESSING, log.logData);
			continue;
		}

		if (queryStu.logLevel != EI_LOG_NONE && !queryStu.function.empty())
		{
			if (log.logLevel == queryStu.logLevel
				&& stringContain(log.logFunction, queryStu.function) >= 0)
				logViewCB(EI_PROCESSING, log.logData);
			continue;
		}

		if (queryStu.logLevel != EI_LOG_NONE && queryStu.timeValid())
		{
			if (log.logLevel == queryStu.logLevel
				&& CompareFileTime(&log.logTime, &queryStu.logStartTime) >= 0
				&& CompareFileTime(&log.logTime, &queryStu.logEndTime) <= 0)
				logViewCB(EI_PROCESSING, log.logData);
			continue;
		}

		if (!queryStu.function.empty() && queryStu.timeValid())
		{
			if (stringContain(log.logFunction, queryStu.function) >= 0
				&& CompareFileTime(&log.logTime, &queryStu.logStartTime) >= 0
				&& CompareFileTime(&log.logTime, &queryStu.logEndTime) <= 0)
				logViewCB(EI_PROCESSING, log.logData);
			continue;
		}

		if (queryStu.logLevel != EI_LOG_NONE || queryStu.timeValid() || !queryStu.function.empty())
		{
			logViewCB(EI_PROCESSING, log.logData);
			continue;
		}
	}
	logViewCB(EI_COMPLETE, "");
}
