#include <iostream>
#include "LogCore/LogCore.h"
#include "Logger/AsyncLogger.h"
#include "LogView/LogPreview.h"

using namespace AsyncLogSDK;
using namespace std;

void queryCallBack(QueryStatus status, const std::string& logData)
{
	std::cout << logData << std::endl;
	if (status == EI_COMPLETE)
		std::cout << "日志查询完成" << std::endl;
}

int main()
{
	CAsyncLogger* pLogger = CAsyncLogger::getInstance();
	if (!pLogger || !pLogger->init())
		return -1;
	
	int i = 0;
	std::chrono::microseconds duration(1000);
	LogQueryStu queryStu;
	while (i < 4000)
	{
		if ((i % 2) != 0)
		{
			LOGERROR("Message: %s, count: %d", "aaa", i++);
		}
		else
		{
			LOGINFO("Message: %s, count: %d", "aaa", i++);
		}

		if (i == 20)
		{
			SYSTEMTIME st;
			GetLocalTime(&st);
			SystemTimeToFileTime(&st, &queryStu.logStartTime);
		}
		std::this_thread::sleep_for(duration);
	}

	std::function<void(QueryStatus status, const std::string&)> LogCallBack = queryCallBack;
	CLogPreview logPreview;
	queryStu.logLevel = EI_LOG_ERROR;
	SYSTEMTIME st;
	GetLocalTime(&st);
	SystemTimeToFileTime(&st, &queryStu.logEndTime);
	logPreview.queryLog(queryStu, LogCallBack);
	getchar();
	return 0;
}