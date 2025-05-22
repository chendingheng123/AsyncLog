#include "LogAlgorithm.h"

namespace AsyncLogSDK
{
    void getNext(int* next, const std::string& s)
    {
        int j = -1;
        next[0] = j;
        for (int i = 1; i < s.size(); i++) 
        {
            while (j >= 0 && s[i] != s[j + 1])  // ע��i��1��ʼ
                j = next[j]; //��ǰ����

            if (s[i] == s[j + 1])
                j++; // �ҵ���ͬ��ǰ��׺

            next[i] = j; // ��j��ǰ׺�ĳ��ȣ�����next[i]
        }
    }

    //����KMP˼�룬��ʵ��strstr���߼����ж�needle�Ƿ���haystack���Ӽ�
    int stringContain(const std::string& haystack, const std::string& needle)
    {
        if (needle.size() == 0)
            return 0;

        std::vector<int> next(needle.size());
        getNext(&next[0], needle);
        int j = -1; // // ��Ϊnext�������¼����ʼλ��Ϊ-1
        for (int i = 0; i < haystack.size(); i++) 
        { 
            // ע��i�ʹ�0��ʼ
            while (j >= 0 && haystack[i] != needle[j + 1]) 
                j = next[j]; // j Ѱ��֮ǰƥ���λ��

            if (haystack[i] == needle[j + 1]) 
                j++; // i��������forѭ����

            if (j == (needle.size() - 1)) 
                return (i - needle.size() + 1);
        }
        return -1;
    }
}

