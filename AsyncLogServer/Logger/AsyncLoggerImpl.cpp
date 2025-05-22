#include "AsyncLoggerImpl.h"
#include <algorithm>
#include <functional>
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
		std::unique_lock<std::mutex> lk(mLogMtx);
		while (mSortedLogList.empty())
		{
			if (::WaitForSingleObject(mExitSem, 0) == WAIT_OBJECT_0)
				return;

			mConsumeCon.wait(lk);
		}
	
		for (const auto& log : mSortedLogList)
		{
			if (!log.logData.empty())
			{
				if (EI_FULL_SIZE == writeFile(log.logData))
				{
					//��ǰ��־�ļ���д����֪ͨBak�߳�ȥ�½�һ����־�ļ�������������ǰ�̵߳�ִ����
					mReadyLogList = std::deque<LogDataBlock>(mSortedLogList.begin() + mWriteIndex, mSortedLogList.end());
					mWriteIndex = 0;
					SetEvent(mBakEvent);
					break;
				}
				mWriteIndex++;
			}
		}
		mSortedLogList.clear();
		mWriteIndex = 0;
	}
}

void CAsyncLoggerImpl::logBakThread()
{
	while (::WaitForSingleObject(mExitSem, 0) != WAIT_OBJECT_0)
	{
		std::vector<FileInfo> fileInfoList;
		getCacheFileList(fileInfoList);
		if (fileInfoList.size() > MAX_LOG_FILE_NUM)
		{
			std::sort(fileInfoList.begin(), fileInfoList.end(),
				[=](const FileInfo& file1, const FileInfo& file2)  //����־�ļ�����ʱ��������������
				{
					return (file1.fileTime.dwHighDateTime < file2.fileTime.dwHighDateTime) ||
						(file1.fileTime.dwHighDateTime == file2.fileTime.dwHighDateTime
							&& file1.fileTime.dwLowDateTime < file2.fileTime.dwLowDateTime);
				});

			//ɾ��ʱ�����iDeleteFileNum����־�ļ�
			int iDeleteFileNum = fileInfoList.size() - MAX_LOG_FILE_NUM;
			for (int i = 0; i < iDeleteFileNum; i++)
				DeleteFileW(fileInfoList[i].filePath.c_str());
			fileInfoList.erase(fileInfoList.begin(), fileInfoList.begin() + iDeleteFileNum);
		}

		if (::WaitForSingleObject(mBakEvent, 0) == WAIT_OBJECT_0)
		{
			createNewLogFile();
			for (const auto& log : mReadyLogList)
			{
				if (!log.logData.empty())
				{
					if (writeFile(log.logData) != EI_NO_ERROR)
						break;
				}
			}
			mReadyLogList.clear();
			ResetEvent(mBakEvent);
		}
		Sleep(1);
	}
}

void CAsyncLoggerImpl::getCacheLogList(std::list<LogDataBlock>& cachedLogList)
{
	cachedLogList = std::list<LogDataBlock>(mCachedLogList.begin(), mCachedLogList.end());
}

void CAsyncLoggerImpl::getCacheFileList(std::vector<FileInfo>& fileInfoList)
{
	std::wstring wsModulePath = getCurrentModulePath();
	searchFilesInDir(wsModulePath.c_str(), fileInfoList);
	if (fileInfoList.size() > MAX_LOG_FILE_NUM)
	{
		std::sort(fileInfoList.begin(), fileInfoList.end(),
			[=](const FileInfo& file1, const FileInfo& file2)
			{
				return (file1.fileTime.dwHighDateTime < file2.fileTime.dwHighDateTime) ||
					(file1.fileTime.dwHighDateTime == file2.fileTime.dwHighDateTime
						&& file1.fileTime.dwLowDateTime < file2.fileTime.dwLowDateTime);
			});

		int iDeleteFileNum = fileInfoList.size() - MAX_LOG_FILE_NUM;
		for (int i = 0; i < iDeleteFileNum; i++)
			DeleteFileW(fileInfoList[i].filePath.c_str());
		fileInfoList.erase(fileInfoList.begin(), fileInfoList.begin() + iDeleteFileNum);
	}
	fileInfoList = fileInfoList;
}

void CAsyncLoggerImpl::createNewLogFile()
{
	char szLogName[128] = { 0 };
	SYSTEMTIME st = { 0 };
	GetLocalTime(&st);
	sprintf(szLogName, "\\%d%d%d%d%d%d%d.log",
		st.wYear, st.wMonth, st.wDay, st.wHour,
		st.wMinute, st.wSecond, st.wMilliseconds);
	std::string logFileName(szLogName);
	std::wstring wsModulePath = getCurrentModulePath();
	std::string sLogPath = wideCharConvertMultiByte(wsModulePath.c_str());
	sLogPath.append(logFileName.c_str());
	mFile = ::CreateFile(sLogPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
}

bool CAsyncLoggerImpl::initLogger()
{
	mExitSem = ::CreateSemaphore(NULL, 0, MAX_LOG_CONSUME_NUM + 1, NULL);
	mBakEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	mBakThread.reset(new std::thread(std::bind(&CAsyncLoggerImpl::logBakThread, this)));
	//������־�����߳�
	mConsumLogThreadVec.resize(mConsumeThreadCnt);
	for (int i = 0; i < mConsumeThreadCnt; ++i)
		mConsumLogThreadVec[i].reset(new std::thread(std::bind(&CAsyncLoggerImpl::logConsumeThread, this)));
	
	std::vector<FileInfo> fileInfoList;
	getCacheFileList(fileInfoList);

	if (fileInfoList.size() > MAX_LOG_FILE_NUM)
	{
		std::sort(fileInfoList.begin(), fileInfoList.end(),
			[=](const FileInfo& file1, const FileInfo& file2)
		{
				return (file1.fileTime.dwHighDateTime < file2.fileTime.dwHighDateTime) ||
					(file1.fileTime.dwHighDateTime == file2.fileTime.dwHighDateTime 
						&& file1.fileTime.dwLowDateTime < file2.fileTime.dwLowDateTime);
		});

		int iDeleteFileNum = fileInfoList.size() - MAX_LOG_FILE_NUM;
		for (int i = 0; i < iDeleteFileNum; i++)
			DeleteFileW(fileInfoList[i].filePath.c_str());
		fileInfoList.erase(fileInfoList.begin(), fileInfoList.begin() + iDeleteFileNum);

		std::string lastestLog = wideCharConvertMultiByte(fileInfoList.back().filePath.c_str());
		mFile = ::CreateFile(lastestLog.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	}

	LARGE_INTEGER size;
	if (!GetFileSizeEx(mFile, &size) || size.QuadPart >= MAX_LOG_FILE_SIZE)
	{
		::CloseHandle(mFile);
		createNewLogFile();
	}
	return mFile != INVALID_HANDLE_VALUE;
}

void CAsyncLoggerImpl::uninitLogger()
{
	//�ѻ������־��ˢ��������ȥ
	if(mReadyLogList.size() > 0)
		SetEvent(mBakEvent);

	ReleaseSemaphore(mExitSem, MAX_LOG_CONSUME_NUM + 1, NULL);
	mConsumeCon.notify_all();

	int iWriteIndex = 0;
	for (const auto& log : mCachedLogList)
	{
		if (writeFile(log.logData) == EI_FULL_SIZE)
		{
			//��������־������������ȿ�������δд��ȥ����־
			mReadyLogList = std::deque<LogDataBlock>(mCachedLogList.begin() + iWriteIndex, mCachedLogList.end());
			createNewLogFile();
			for (const auto& log : mReadyLogList)
			{
				if (!log.logData.empty())
				{
					if (writeFile(log.logData) != EI_NO_ERROR)
						break;
				}
			}
			mReadyLogList.clear();
			break;
		}
		++iWriteIndex;
	}

	if (mBakThread->joinable())
		mBakThread->join();

	for (auto& iter : mConsumLogThreadVec)
	{
		if (iter->joinable())
			iter->join();
	}

	if (mFile != INVALID_HANDLE_VALUE)
		::CloseHandle(mFile);
	if (mExitSem != INVALID_HANDLE_VALUE)
		::CloseHandle(mExitSem);
}

ErrorCode CAsyncLoggerImpl::writeFile(const std::string& sContent)
{
	if (mFile == INVALID_HANDLE_VALUE)
		return EI_WRITE_ERROR;

	LARGE_INTEGER size;
	if (!GetFileSizeEx(mFile, &size) || size.QuadPart >= MAX_LOG_FILE_SIZE)
	{
		//������ָ����С����ʱ���ܼ��������ļ���д�ˣ���Ҫ��д�µ��ļ�
		::CloseHandle(mFile);
		return EI_FULL_SIZE;
	}

	DWORD dwByteWritten = 0;
	if (!WriteFile(mFile, sContent.c_str(), sContent.size(), &dwByteWritten, nullptr) || dwByteWritten < sContent.size())
		return EI_WRITE_ERROR;

	return FlushFileBuffers(mFile) ? EI_NO_ERROR : EI_WRITE_ERROR;
}

void CAsyncLoggerImpl::writeLog(LogLevel logLevel, PCSTR functionName, int lineNo, const std::string& sFormatMsg)
{
	if (logLevel < LogLevel::EI_LOG_INFO)
		return;

	char szTime[128] = { 0 };
	SYSTEMTIME st = { 0 };
	GetLocalTime(&st);
	sprintf(szTime, "[%d-%d-%d %d:%d:%d:%d]", 
			st.wYear, st.wMonth, st.wDay, st.wHour, 
			st.wMinute, st.wSecond, st.wMilliseconds);
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
	sprintf_s(szFunctionSig, "[%s:%d]", functionName, lineNo);
	sFilterInfo += szFunctionSig;

	std::string sFullLogMsg;
	sFullLogMsg.append(sFormatMsg.c_str(), sFormatMsg.size());//�����ַ���ĩβ�Ľ�����"\0"
	sFilterInfo += sFullLogMsg;
	sFilterInfo += "\r\n";

	LogDataBlock dataBlock;
	dataBlock.logData = sFilterInfo;
	auto timeNow = std::chrono::system_clock::now();
	FILETIME fileTime;
	if (SystemTimeToFileTime(&st, &fileTime))
		dataBlock.logTime = fileTime;
	dataBlock.logLevel = logLevel;
	dataBlock.logFunction = functionName;
	std::unique_lock<std::mutex> lk(mLogMtx);
	mCachedLogList.push_back(dataBlock);
	if (mCachedLogList.size() > MAX_LOG_PRODUCE_NUM)
	{
		//��mSortedLogList��100����־���ݽ�������
		mSortedLogList = std::deque<LogDataBlock>(mCachedLogList.begin(), mCachedLogList.begin() + MAX_LOG_PRODUCE_NUM);
		mCachedLogList.erase(mCachedLogList.begin(), mCachedLogList.begin() + MAX_LOG_PRODUCE_NUM);
		std::sort(mSortedLogList.begin(), mSortedLogList.end(),
			[&](const LogDataBlock& data1, const LogDataBlock& data2)
			{
				return CompareFileTime(&data1.logTime, &data2.logTime) < 0;
			});
		//����������־������100��, ֪ͨ��־�����̣߳�����־д����־�ļ���ȥ
		mConsumeCon.notify_one();
	}
}


