#ifndef C_LOG_ALGORITHM_H_
#define C_LOG_ALGORITHM_H_

#include "../LogCore/Global.h"

namespace AsyncLogSDK
{
	void getNext(int* next, const std::string& s);
	int stringContain(const std::string& haystack, const std::string& needle);
}

#endif // !C_LOG_ALGORITHM_H_