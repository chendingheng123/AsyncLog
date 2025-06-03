#include "Global.h"

namespace AsyncLogSDK
{
	std::string wideCharConvertMultiByte(LPCWSTR lpcwStr)
	{
		if (!lpcwStr)
			return "";

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
	}

	std::wstring multiByteConvertWideChar(const std::string& str)
	{
		int wsLen = MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()), NULL, 0);
		std::wstring wsResult(wsLen, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], static_cast<int>(str.size()), &wsResult[0], wsLen);
		return wsResult;
	}

	std::wstring getCurrentModulePath()
	{
		WCHAR szModulePath[MAX_PATH] = { '\0' };
		if (GetModuleFileNameW(NULL, szModulePath, MAX_PATH) > 0)
		{
			std::wstring wsModulePath = szModulePath;
			size_t iLastSlashPos = wsModulePath.find_last_of(L"\\/");
			if (iLastSlashPos != std::wstring::npos)
				return wsModulePath.substr(0, iLastSlashPos);
		}
		return std::wstring();
	}

	void searchFilesInDir(LPCWSTR sDirPath, std::vector<FileInfo>& fileInfoList)
	{
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
	}

	bool directionaryExist(const std::string& dirPath)
	{
		DWORD attribute = GetFileAttributes(dirPath.c_str());
		return attribute != INVALID_FILE_ATTRIBUTES && (attribute & FILE_ATTRIBUTE_DIRECTORY);
	}
}