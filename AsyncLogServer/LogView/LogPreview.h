#ifndef C_LOG_PREVIEW_H_
#define C_LOG_PREVIEW_H_
#include "../LogCore/Global.h"
#include <functional>

namespace AsyncLogSDK
{
	typedef std::function<void(QueryStatus status, const std::string&)> LogViewCallBack;
	class AsyncLogApi CLogPreview
	{	
		public:
			CLogPreview();
			~CLogPreview();
		
		public:
			void queryLog(LogQueryStu& queryStu, LogViewCallBack& logViewCB);
		
		protected:
			void queryLogThread(LogQueryStu& queryStu, LogViewCallBack& logViewCB);

		private:
			std::unique_ptr<std::thread> spQueryThread;
	};
}

#endif // !C_LOG_PREVIEW_H_