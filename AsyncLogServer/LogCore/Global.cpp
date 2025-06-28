#include "Global.h"
#include <string>

namespace AsyncLogSDK
{
	std::string wideCharConvertMultiByte(LPCWSTR lpcwStr)
	{
		if (!lpcwStr)
			return "";

#if defined(_WIN32) || defined(_WIN64)
		int iLen = WideCharToMultiByte(CP_UTF8, 0, lpcwStr, -1, nullptr, 0, nullptr, nullptr);
		if (iLen == 0)
			return "";

		char* pConvertRes = new char[iLen + 1];
		if (WideCharToMultiByte(CP_UTF8, 0, lpcwStr, -1, pConvertRes, iLen, nullptr, nullptr) == 0)
			return "";

		std::string sConvertRes = pConvertRes;
		delete[] pConvertRes;
		pConvertRes = nullptr;
		return sConvertRes;
#elif defined(__linux__)
		setlocale(LC_ALL, "en_US.UTF-8");  // 设置本地化环境（确保宽字符转换正确）

		//计算所需缓冲区大小
		size_t len = wcslen(lpcwStr) * 4 + 1; // UTF-8最多占4字节/字符
		char* buffer = new char[len];

		//执行转换
		size_t converted = wcstombs(buffer, lpcwStr, len);
		std::string result(buffer, converted);

		delete[] buffer;
		return result;
#endif
	}

	std::wstring multiByteConvertWideChar(const std::string& str)
	{
#if defined(_WIN32) || defined(_WIN64)
		int wsLen = MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()), NULL, 0);
		std::wstring wsResult(wsLen, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()), &wsResult[0], wsLen);
		return wsResult;
#elif defined(__linux__)
		if (str.empty())
			return L"";

		//设置UTF-8本地化
		setlocale(LC_ALL, "en_US.UTF-8");
		//计算所需宽字符长度
		size_t len = mbstowcs(nullptr, str.c_str(), 0) + 1;
		wchar_t* buffer = new wchar_t[len];

		mbstowcs(buffer, str.c_str(), len);
		std::wstring result(buffer);
		delete[] buffer;
		return result;
#endif
	}

	std::wstring getCurrentModulePath()
	{
#if defined(_WIN32) || defined(_WIN64)
		WCHAR szModulePath[MAX_PATH] = { '\0' };
		if (GetModuleFileNameW(NULL, szModulePath, MAX_PATH) > 0)
		{
			std::wstring wsModulePath = szModulePath;
			size_t iLastSlashPos = wsModulePath.find_last_of(L"\\/");
			if (iLastSlashPos != std::wstring::npos)
				return wsModulePath.substr(0, iLastSlashPos);
		}
		return std::wstring();
#elif defined(__linux__)
		char path[PATH_MAX];
		ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
		std::string sLogDir;
		if (len != -1)
		{
			sLogDir = path;
			size_t nPos = sLogDir.find_last_of("/");
			if (nPos != std::string::npos)
				sLogDir = sLogDir.substr(0, nPos);

			sLogDir.append("/LogServer");
			struct stat info;
			if (stat(sLogDir.c_str(), &info) != 0)
			{
				if (mkdir(sLogDir.c_str(), 0755) == 0)
					return multiByteConvertWideChar(sLogDir);
				return L"";
			}
		}
		return multiByteConvertWideChar(sLogDir);
#endif
	}

	void searchFilesInDir(LPCWSTR sDirPath, std::vector<FileInfo>& fileInfoList)
	{
#if defined(_WIN32) || defined(_WIN64)
		std::wstring wsImgDir(sDirPath);
		wsImgDir.append(L"\\*");
		WIN32_FIND_DATAW winFileData = { 0 };
		HANDLE hFileHandle = FindFirstFileW(wsImgDir.c_str(), &winFileData);
		if (hFileHandle == INVALID_HANDLE_VALUE)
			return;

		wsImgDir = wsImgDir.substr(0, wsImgDir.size() - 1);
		do
		{
			if (!(winFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				std::wstring wsImgPath = wsImgDir;
				wsImgPath.append(winFileData.cFileName);
				std::wstring wsTemp = L".log";
				if (wsImgPath.find(wsTemp) != std::wstring::npos)
				{
					FileInfo fileInfo;
					fileInfo.filePath = wsImgPath;
					fileInfo.fileTime = winFileData.ftCreationTime;
					fileInfoList.emplace_back(fileInfo);
				}
			}
		} while (FindNextFileW(hFileHandle, &winFileData) != 0);
		FindClose(hFileHandle);
#elif defined(__linux__)
		std::string sLogDirPath = wideCharConvertMultiByte(sDirPath);
		DIR* dir = opendir(sLogDirPath.c_str());
		if (dir == nullptr)
			return;

		struct dirent* entry;
		while ((entry = readdir(dir)) != nullptr)
		{
			std::string name = entry->d_name;
			if (name == "." || name == "..")
				continue;

			std::string fullPath = sLogDirPath + "/" + name;
			struct stat st;
			if (stat(fullPath.c_str(), &st) == 0 && S_ISREG(st.st_mode)
				&& strstr(fullPath.c_str(), ".log") != nullptr && strstr(fullPath.c_str(), ".log.swp") == nullptr)
			{
				FileInfo fileInfo;
				fileInfo.filePath = fullPath;
				struct stat fileStat;
				if (stat(fullPath.c_str(), &fileStat) == 0)
				{
					//std::cout << "fileTime: " << fileStat.st_ctime << "  fileName: " << fullPath << std::endl;
					fileInfo.fileTime = fileStat.st_ctime;
				}
				fileInfoList.emplace_back(fileInfo);
			}
		}
		closedir(dir);
#endif
	}

	bool directionaryExist(const std::string& dirPath)
	{
#if defined(_WIN32) || defined(_WIN64)
		DWORD attribute = GetFileAttributes(dirPath.c_str());
		return attribute != INVALID_FILE_ATTRIBUTES && (attribute & FILE_ATTRIBUTE_DIRECTORY);
#elif defined(__linux__)
		struct stat info = { 0 };
		if (stat(dirPath.c_str(), &info) != 0)
			return true;
		return false;
#endif
	}
}