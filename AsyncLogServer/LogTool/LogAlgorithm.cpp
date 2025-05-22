#include "LogAlgorithm.h"

namespace AsyncLogSDK
{
    void getNext(int* next, const std::string& s)
    {
        int j = -1;
        next[0] = j;
        for (int i = 1; i < s.size(); i++) 
        {
            while (j >= 0 && s[i] != s[j + 1])  // 注意i从1开始
                j = next[j]; //向前回退

            if (s[i] == s[j + 1])
                j++; // 找到相同的前后缀

            next[i] = j; // 将j（前缀的长度）赋给next[i]
        }
    }

    //借助KMP思想，自实现strstr的逻辑，判断needle是否是haystack的子集
    int stringContain(const std::string& haystack, const std::string& needle)
    {
        if (needle.size() == 0)
            return 0;

        std::vector<int> next(needle.size());
        getNext(&next[0], needle);
        int j = -1; // // 因为next数组里记录的起始位置为-1
        for (int i = 0; i < haystack.size(); i++) 
        { 
            // 注意i就从0开始
            while (j >= 0 && haystack[i] != needle[j + 1]) 
                j = next[j]; // j 寻找之前匹配的位置

            if (haystack[i] == needle[j + 1]) 
                j++; // i的增加在for循环里

            if (j == (needle.size() - 1)) 
                return (i - needle.size() + 1);
        }
        return -1;
    }
}

