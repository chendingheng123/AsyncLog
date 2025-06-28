#include "AsyncLoggerImpl.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <functional>
#include "../LogTool/LogAlgorithm.h"
using namespace AsyncLogSDK;

CAsyncLoggerImpl::CAsyncLoggerImpl()
{
}

CAsyncLoggerImpl::~CAsyncLoggerImpl()
{
}

void CAsyncLoggerImpl::logConsumeThread()
{
	while (true)
	{
		int iError = EI_NO_ERROR;
		std::unique_lock<std::mutex> lk(mLogMtx);
		while (mSortedLogList.empty() || iError == EI_WRITE_ERROR)
			mConsumeCon.wait(lk);

		while (!mbCreateNewFile.load(std::memory_order_relaxed))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
#if defined(_WIN32) || defined(_WIN64)
			if (::WaitForSingleObject(mExitSem, 0) == WAIT_OBJECT_0)
				return;
#elif defined(__linux__)
			if (sem_trywait(&mExitSem) == 0)
			{
				std::cout << "consume thread exit." << std::endl;
				return;
			}
#endif
		}

		for (const auto& log : mSortedLogList)
		{
			if (!log.logData.empty())
			{
				iError = writeFile(log.logData);
				if (EI_FULL_SIZE == iError)
				{
					//��ǰ��־�ļ���д����֪ͨBak�߳�ȥ�½�һ����־�ļ�������������ǰ�̵߳�ִ����
					{
						mSortedLogList.erase(mSortedLogList.begin(), mSortedLogList.begin() + mWriteIndex);
						mWriteIndex = 0;
					}
#if defined(_WIN32) || defined(_WIN64)
					SetEvent(mBakEvent);
#elif defined(__linux__)
					mbCreateNewFile.store(false, std::memory_order_relaxed); // ���ñ�־λΪfalse
					sem_post(&mBakEvent);
#endif
					break;
				}
				else if (EI_WRITE_ERROR == iError)
				{
					if (mWriteIndex > 0)
					{
						mSortedLogList.erase(mSortedLogList.begin(), mSortedLogList.begin() + mWriteIndex);
						mWriteIndex = 0;
					}
					break;
				}
				mWriteIndex++;
			}
		}
		//�����д����������bak�̴߳�����־�ļ�����ʱ����ʱû�����ļ���д��־
		if (iError == EI_WRITE_ERROR)
			continue;

		if (iError == EI_NO_ERROR)
			mSortedLogList.clear();
		mWriteIndex = 0;
	}
}

void CAsyncLoggerImpl::logBakThread()
{
#if defined(_WIN32) || defined(_WIN64)
	while (::WaitForSingleObject(mExitSem, 0) != WAIT_OBJECT_0)
#elif defined(__linux__)
	while (sem_trywait(&mExitSem) != 0)
#endif
	{
#if defined(_WIN32) || defined(_WIN64)
		{
			std::vector<FileInfo> fileInfoList;
			if (getCacheFileList(fileInfoList) && fileInfoList.size() > MAX_LOG_FILE_NUM)
			{
				//ɾ��ʱ�����iDeleteFileNum����־�ļ�
				int iDeleteFileNum = static_cast<int>(fileInfoList.size() - MAX_LOG_FILE_NUM);
				for (int i = 0; i < iDeleteFileNum; i++)
					DeleteFileW(fileInfoList[i].filePath.c_str());

				fileInfoList.erase(fileInfoList.begin(), fileInfoList.begin() + iDeleteFileNum);
			}
		}
#elif defined(__linux__)
		{
			std::unique_lock<std::mutex> lk(mLogPathMtx);
			if (mLogPathArr.size() > MAX_LOG_FILE_NUM)
			{
				//ɾ��ʱ�����iDeleteFileNum����־�ļ�
				int iDeleteFileNum = mLogPathArr.size() - MAX_LOG_FILE_NUM;
				for (int i = 0; i < iDeleteFileNum; i++)
					remove(mLogPathArr[i].c_str());

				mLogPathArr.erase(mLogPathArr.begin(), mLogPathArr.begin() + iDeleteFileNum);
			}
		}
#endif

#if defined(_WIN32) || defined(_WIN64)
		if (::WaitForSingleObject(mBakEvent, 0) == WAIT_OBJECT_0)
#elif defined(__linux__)
		if (sem_wait(&mBakEvent) == 0)
#endif
		{
			if (createNewLogFile())
				mbCreateNewFile.store(true, std::memory_order_relaxed); // ���ñ�־λΪfalse
#ifdef _WIN32
			ResetEvent(mBakEvent);
#endif
		}
#if defined(_WIN32) || defined(_WIN64)
		Sleep(1);
#elif defined(__linux__)
		usleep(1);
#endif
	}
}

void CAsyncLoggerImpl::getCacheLogList(std::list<LogDataBlock>& cachedLogList)
{
	cachedLogList = std::list<LogDataBlock>(mCachedLogList.begin(), mCachedLogList.end());
}

bool CAsyncLoggerImpl::getCacheFileList(std::vector<FileInfo>& fileInfoList)
{
	std::wstring wsModulePath = getCurrentModulePath();
	std::string sLogDir = wideCharConvertMultiByte(wsModulePath.c_str());
#ifdef _WIN32
	//ƴ�ϵ�ǰ������
	char processName[128] = { 0 };
	std::string sProcessName;
	if (::GetModuleFileName(NULL, processName, 128))
		sProcessName = processName;
	if (sProcessName.empty())
		return false;

	if (stringContain(sProcessName, sLogDir) >= 0)
		sProcessName = sProcessName.substr(sLogDir.size(), sProcessName.size() - 1);
	size_t index = sProcessName.find_last_of(".");
	if (index != std::string::npos)
		sProcessName = sProcessName.substr(0, index);
	sLogDir.append(sProcessName);
#endif

	std::wstring wsLogDir = multiByteConvertWideChar(sLogDir);
	searchFilesInDir(wsLogDir.c_str(), fileInfoList);
	std::sort(fileInfoList.begin(), fileInfoList.end(),
		[=](const FileInfo& file1, const FileInfo& file2)
		{
#if defined(_WIN32) || defined(_WIN64)
			return (file1.fileTime.dwHighDateTime < file2.fileTime.dwHighDateTime) ||
				(file1.fileTime.dwHighDateTime == file2.fileTime.dwHighDateTime
					&& file1.fileTime.dwLowDateTime < file2.fileTime.dwLowDateTime);
#elif defined(__linux__)
			return 	file1.fileTime <= file2.fileTime
				&& file1.filePath < file2.filePath;
#endif
		});
	return true;
}

bool CAsyncLoggerImpl::createNewLogFile()
{
	char szLogName[128] = { 0 };
#if defined(_WIN32) || (_WIN64)
	SYSTEMTIME st = { 0 };
	GetLocalTime(&st);
	sprintf(szLogName, "\\%d%d%d%d%d%d%d.log",
		st.wYear, st.wMonth, st.wDay, st.wHour,
		st.wMinute, st.wSecond, st.wMilliseconds);
#elif defined(__linux__)
	auto nowTime = std::chrono::system_clock::now();
	auto nowTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime.time_since_epoch()) % 1000;
	// ת��Ϊ����ʱ�䣨�̰߳�ȫ�汾��
	std::time_t st = std::chrono::system_clock::to_time_t(nowTime);
	struct tm tm_buf;
	localtime_r(&st, &tm_buf);
	// ��ϳ�Ŀ���ʽ
	strftime(szLogName, sizeof(szLogName), "/%Y%m%d%H%M%S", &tm_buf);
	std::string sTemp = szLogName;
	sprintf(szLogName + sTemp.size(), "%04ld.log", nowTimeMs.count());
#endif
	std::string logFileName(szLogName);
	std::wstring wsModulePath = getCurrentModulePath();
	std::string sLogPath = wideCharConvertMultiByte(wsModulePath.c_str());
#if defined(_WIN32) || (_WIN64)
	//ƴ�ϵ�ǰ������
	char processName[128] = { 0 };
	::GetModuleFileName(NULL, processName, 128);
	std::string sProcessName = processName;
	if (stringContain(sProcessName, sLogPath) >= 0)
		sProcessName = sProcessName.substr(sLogPath.size(), sProcessName.size() - 1);
	size_t index = sProcessName.find_last_of(".");
	if (index != std::string::npos)
		sProcessName = sProcessName.substr(0, index);

	sLogPath.append(sProcessName);
	if (!directionaryExist(sLogPath))
		::CreateDirectory(sLogPath.c_str(), NULL);
#endif

	sLogPath.append(logFileName.c_str());
	std::unique_lock<std::mutex> lk(mFileHandleMtx);
#if defined(_WIN32) || (_WIN64)
	mFile = ::CreateFile(sLogPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	return mFile != INVALID_HANDLE_VALUE;
#elif defined(__linux__)
	mFile.open(sLogPath.c_str(), std::ios::in | std::ios::app | std::ios::binary);
	if (mFile.is_open())
	{
		std::unique_lock<std::mutex> pathLock(mLogPathMtx);
		mLogPathArr.push_back(sLogPath);
	}
	return mFile.is_open();
#endif
}

bool CAsyncLoggerImpl::initLogger()
{
#if defined(_WIN32) || (_WIN64)
	mExitSem = ::CreateSemaphore(NULL, 0, MAX_LOG_CONSUME_NUM + 1, NULL);
	mBakEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
#elif defined(__linux__)
	sem_init(&mExitSem, 0, 0);
	sem_init(&mBakEvent, 0, 0);
#endif
	mBakThread.reset(new std::thread(std::bind(&CAsyncLoggerImpl::logBakThread, this)));
	//������־�����߳�
	mConsumLogThreadVec.resize(mConsumeThreadCnt);
	for (int i = 0; i < mConsumeThreadCnt; ++i)
		mConsumLogThreadVec[i].reset(new std::thread(std::bind(&CAsyncLoggerImpl::logConsumeThread, this)));

	std::vector<FileInfo> fileInfoList;
	if (!getCacheFileList(fileInfoList))
		return false;

	std::string lastestLog;
	if (fileInfoList.size() > MAX_LOG_FILE_NUM)
	{
		int iDeleteFileNum = fileInfoList.size() - MAX_LOG_FILE_NUM;
		for (int i = 0; i < iDeleteFileNum; i++)
		{
#if defined(_WIN32) || defined(_WIN64)
			DeleteFileW(fileInfoList[i].filePath.c_str());
#elif defined(__linux__)
			remove(fileInfoList[i].filePath.c_str());
#endif
		}

		fileInfoList.erase(fileInfoList.begin(), fileInfoList.begin() + iDeleteFileNum);
#if defined(_WIN32) || (_WIN64)
		lastestLog = wideCharConvertMultiByte(fileInfoList.back().filePath.c_str());
		mFile = ::CreateFile(lastestLog.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
#elif defined(__linux__)
		lastestLog = fileInfoList.back().filePath;
		mFile.open(lastestLog.c_str(), std::ios::in | std::ios::app | std::ios::binary);
#endif
	}

#if defined(__linux__)
	{
		//ÿ����־���ʼ��ʱ����Ҫ�����µ�ǰ�ڴ���mLogPathArr��ֵ
		std::unique_lock<std::mutex> pathLock(mLogPathMtx);
		int iSize = static_cast<int>(fileInfoList.size());
		for (int i = iSize - 1; i >= 0; i--)
			mLogPathArr.emplace(mLogPathArr.begin(), fileInfoList[i].filePath);
	}
#endif

#if defined(_WIN32) || (_WIN64)
	LARGE_INTEGER size;
	if (!GetFileSizeEx(mFile, &size) || size.QuadPart >= MAX_LOG_FILE_SIZE)
	{
		::CloseHandle(mFile);
		createNewLogFile();
	}
	return mFile != INVALID_HANDLE_VALUE;
#elif defined(__linux__)
	if (fileInfoList.size() > 0)
	{
		lastestLog = fileInfoList.back().filePath;
		mFile.open(lastestLog.c_str(), std::ios::in | std::ios::app | std::ios::binary);
		struct stat fileStat;
		if (stat(lastestLog.c_str(), &fileStat) == 0)
		{
			if (fileStat.st_size > MAX_LOG_FILE_SIZE)
			{
				mFile.close();
				createNewLogFile();
			}
			return mFile.is_open();
		}
		return false;
	}
	else
	{
		createNewLogFile();
		return mFile.is_open();
	}
#endif
}

void CAsyncLoggerImpl::uninitLogger()
{
	//�˳��������߳�
#if defined(_WIN32) || (_WIN64)
	ReleaseSemaphore(mExitSem, MAX_LOG_CONSUME_NUM + 1, NULL);
#elif defined(__linux__)
	int iConsumeThreadSize = static_cast<int>(mConsumLogThreadVec.size());
	for (int i = 0; i < iConsumeThreadSize + 1; i++)
		sem_post(&mExitSem);
#endif
	mConsumeCon.notify_all();

	//��mCachedLogListʣ������ݰ���ʱ������ƴ�ӵ��������̻߳�û���ü������mSortedLogList��ȥ
	if (mCachedLogList.size() > 0)
	{
		auto sortedLogList = std::deque<LogDataBlock>(mCachedLogList.begin(), mCachedLogList.end());
		std::sort(sortedLogList.begin(), sortedLogList.end(),
			[&](const LogDataBlock& data1, const LogDataBlock& data2)
			{
#if defined(_WIN32) || (_WIN64)	
				return CompareFileTime(&data1.logTime, &data2.logTime) < 0;
#elif defined(__linux__)
				return data1.logTime < data2.logTime;
#endif
			});
		//��mSortedLogList��100����־���ݽ�������
		if (mSortedLogList.size() > 0)
			mSortedLogList.insert(mSortedLogList.end(), sortedLogList.begin(), sortedLogList.end());
		else
			std::swap(mSortedLogList, sortedLogList);
	}

	int iWriteIndex = 0;
	for (const auto& log : mSortedLogList)
	{
		if (writeFile(log.logData) == EI_FULL_SIZE)
		{
			//��������־������������ȿ�������δд��ȥ����־
			auto readyLogList = std::deque<LogDataBlock>(mSortedLogList.begin() + iWriteIndex, mSortedLogList.end());
			createNewLogFile();
			for (const auto& log : readyLogList)
			{
				if (!log.logData.empty())
				{
					if (writeFile(log.logData) != EI_NO_ERROR)
						break;
				}
			}
			readyLogList.clear();
			break;
		}
		++iWriteIndex;
	}

	//��־�ļ�����ʼ��ά����10��
	std::vector<FileInfo> fileInfoList;
	getCacheFileList(fileInfoList);
	if (fileInfoList.size() > MAX_LOG_FILE_NUM)
	{
		int iDeleteFileNum = fileInfoList.size() - MAX_LOG_FILE_NUM;
		for (int i = 0; i < iDeleteFileNum; i++)
		{
#if defined(_WIN32) || defined(_WIN64)
			DeleteFileW(fileInfoList[i].filePath.c_str());
#elif defined(__linux__)
			remove(fileInfoList[i].filePath.c_str());
#endif
		}
	}

	if (mBakThread->joinable())
		mBakThread->join();

	for (auto& iter : mConsumLogThreadVec)
	{
		if (iter->joinable())
			iter->join();
	}

	{
		std::unique_lock<std::mutex> lk(mFileHandleMtx);
#if defined(_WIN32) || (_WIN64)
		if (mFile != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(mFile);
			mFile = INVALID_HANDLE_VALUE;
		}

		if (mExitSem != INVALID_HANDLE_VALUE)
			::CloseHandle(mExitSem);
#elif defined(__linux__)
		if (mFile.is_open())
		{
			mFile.close();
		}
		sem_close(&mExitSem);
#endif
	}
}

ErrorCode CAsyncLoggerImpl::writeFile(const std::string& sContent)
{
	std::unique_lock<std::mutex> lk(mFileHandleMtx);
#if defined(_WIN32) || (_WIN64)
	if (mFile == INVALID_HANDLE_VALUE)
		return EI_WRITE_ERROR;

	LARGE_INTEGER size;
	if (!GetFileSizeEx(mFile, &size) || size.QuadPart >= MAX_LOG_FILE_SIZE)
	{
		//������ָ����С����ʱ���ܼ��������ļ���д�ˣ���Ҫ��д�µ��ļ�
		::CloseHandle(mFile);
		mFile = INVALID_HANDLE_VALUE;
		return EI_FULL_SIZE;
	}

	DWORD dwByteWritten = 0;
	if (!WriteFile(mFile, sContent.c_str(), sContent.size(), &dwByteWritten, nullptr) || dwByteWritten < sContent.size())
		return EI_WRITE_ERROR;

	return FlushFileBuffers(mFile) ? EI_NO_ERROR : EI_WRITE_ERROR;

#elif defined(__linux__)

	std::string lastestFilePath;
	{
		std::unique_lock<std::mutex> lk(mLogPathMtx);
		lastestFilePath = mLogPathArr.back();
		struct stat fileStat;
		if (stat(lastestFilePath.c_str(), &fileStat) == 0)
		{
			if (fileStat.st_size > MAX_LOG_FILE_SIZE)
			{
				std::cout << lastestFilePath << " exceed size limit." << std::endl;
				mFile.close();
				return EI_FULL_SIZE;
			}
		}
	}

	if (!mFile.is_open() && !lastestFilePath.empty())
		mFile.open(lastestFilePath.c_str(), std::ios::in | std::ios::app | std::ios::binary);

	mFile << sContent;
	mFile.flush();
	return EI_NO_ERROR;
#endif
}

void CAsyncLoggerImpl::writeLog(LogLevel logLevel, PCSTR functionName, int lineNo, const std::string& sFormatMsg)
{
	if (logLevel < LogLevel::EI_LOG_INFO)
		return;

	char szTime[128] = { 0 };
	double dTime = 0;
#if defined(_WIN32) || (_WIN64)
	SYSTEMTIME st = { 0 };
	GetLocalTime(&st);
	sprintf(szTime, "[%d-%d-%d %d:%d:%d:%d]",
		st.wYear, st.wMonth, st.wDay, st.wHour,
		st.wMinute, st.wSecond, st.wMilliseconds);
#elif defined(__linux__)
	auto nowTime = std::chrono::system_clock::now();
	auto nowTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime.time_since_epoch()) % 1000;

	// ת��Ϊ����ʱ�䣨�̰߳�ȫ�汾��
	std::time_t st = std::chrono::system_clock::to_time_t(nowTime);
	struct tm tm_buf;
	localtime_r(&st, &tm_buf);
	// ��ϳ�Ŀ���ʽ
	strftime(szTime, sizeof(szTime), "[%Y-%m-%d %H:%M:%S", &tm_buf);
	std::string sTemp = szTime;
	sprintf(szTime + sTemp.size(), ":%ld]", nowTimeMs.count());

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	dTime = static_cast<double>(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif

	std::string sFilterInfo(szTime);

	//���󼶱�
	std::string sLevel("[Info]");
	if (logLevel == EI_LOG_WARNING)
		sLevel = "[Warning]";
	else if (logLevel == EI_LOG_ERROR)
		sLevel = "[Error]";
	else if (logLevel == EI_LOG_FATAL)
		sLevel = "[Fatal]";
	sFilterInfo += sLevel;

	//����������ǩ�����к���Ϣ
	char szFunctionSig[128] = { 0 };
#if defined(_WIN32) || (_WIN64)
	sprintf_s(szFunctionSig, "[%s:%d]", functionName, lineNo);
#elif defined(__linux__)
	sprintf(szFunctionSig, "[%s:%d]", functionName, lineNo);
#endif
	sFilterInfo += szFunctionSig;

	std::string sFullLogMsg;
	sFullLogMsg.append(sFormatMsg.c_str(), sFormatMsg.size());//�����ַ���ĩβ�Ľ�����"\0"
	sFilterInfo += sFullLogMsg;
	sFilterInfo += "\r\n";

	LogDataBlock dataBlock;
	dataBlock.logData = sFilterInfo;
#if defined(_WIN32) || (_WIN64)	
	FILETIME fileTime;
	if (SystemTimeToFileTime(&st, &fileTime))
		dataBlock.logTime = fileTime;
#elif defined(__linux__)
	dataBlock.logTime = dTime;
#endif
	dataBlock.logLevel = logLevel;
	dataBlock.logFunction = functionName;
	std::unique_lock<std::mutex> lk(mLogMtx);
	mCachedLogList.push_back(dataBlock);
	if (mCachedLogList.size() > MAX_LOG_PRODUCE_NUM)
	{
		auto sortedLogList = std::deque<LogDataBlock>(mCachedLogList.begin(), mCachedLogList.begin() + MAX_LOG_PRODUCE_NUM);
		mCachedLogList.erase(mCachedLogList.begin(), mCachedLogList.begin() + MAX_LOG_PRODUCE_NUM);
		std::sort(sortedLogList.begin(), sortedLogList.end(),
			[&](const LogDataBlock& data1, const LogDataBlock& data2)
			{
#if defined(_WIN32) || (_WIN64)	
				return CompareFileTime(&data1.logTime, &data2.logTime) < 0;
#elif defined(__linux__)
				return data1.logTime < data2.logTime;
#endif
			});

		//��mSortedLogList��100����־���ݽ�������
		if (mSortedLogList.size() > 0)
			mSortedLogList.insert(mSortedLogList.end(), sortedLogList.begin(), sortedLogList.end());
		else
			std::swap(mSortedLogList, sortedLogList);

		//����������־������100��, ֪ͨ��־�����̣߳�����־д����־�ļ���ȥ
		mConsumeCon.notify_one();
	}
}