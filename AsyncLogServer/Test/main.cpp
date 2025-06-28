#include "../Logger/AsyncLogger.h"
#include <iostream>
#include "../LogCore/LogCore.h"
#include "../LogView/LogPreview.h"

using namespace AsyncLogSDK;
using namespace std;

void queryCallBack(QueryStatus status, const std::string& logData)
{
    std::cout << logData << std::endl;
    if (status == EI_COMPLETE)
        std::cout << "日志查询完成" << std::endl;
}

#if defined(_WIN32) || defined(_WIN64)
class LogBenchmark {
public:
    struct TestResult {
        size_t total_bytes;
        double duration_sec;
        size_t threads_count;
    };


    static TestResult throughput_test(int duration_sec, size_t thread_count) {
        std::atomic<bool> running(true);
        std::atomic<size_t> counter(0);
        std::vector<std::thread> threads;


        auto start = std::chrono::high_resolution_clock::now();

        // 创建线程
        for (size_t i = 0; i < thread_count; ++i) {
            threads.emplace_back([&]() {
                std::string message(1024, 'X'); // 1KB每条日志
                while (running) {
                    LOGINFO(message.c_str());
                    counter += message.size();
                }
                });
        }


        // 运行指定时间
        std::this_thread::sleep_for(std::chrono::seconds(duration_sec));
        running = false;


        // 等待线程结束
        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }


        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double>(end - start).count();


        return { counter.load(), duration, thread_count };
    }


    static TestResult latency_test(size_t target_bytes) {
        const std::string message(1024, 'X'); // 1KB每条日志
        const size_t messages_needed = target_bytes / message.size();


        auto start = std::chrono::high_resolution_clock::now();


        for (size_t i = 0; i < messages_needed; ++i) {
            LOGINFO(message.c_str());
        }


        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double>(end - start).count();


        return { messages_needed * message.size(), duration, 1 };
    }


    static void run_tests() {
        // 测试1: 30秒吞吐量测试（多线程）
        std::cout << "=== Throughput Test (30s) ===" << std::endl;
        for (size_t threads : {1, 4, 8}) {
            auto result = throughput_test(30, threads);
            print_result(result, "Throughput");
        }


        // 测试2: 10MB延迟测试
        std::cout << "\n=== Latency Test (10MB) ===" << std::endl;
        auto result = latency_test(10 * 1024 * 1024);
        print_result(result, "Latency");


        // 测试3: 线程竞争分析
        std::cout << "\n=== Thread Contention Analysis ===" << std::endl;
        for (size_t threads : {1, 2, 4, 8, 16}) {
            auto result = throughput_test(5, threads);
            print_scalability(result);
        }
    }


private:
    static void print_result(const TestResult& res, const std::string& name) {
        double throughput = res.total_bytes / res.duration_sec / (1024 * 1024);
        std::cout << name << " (" << res.threads_count << " threads):\n"
            << "  Total: " << res.total_bytes / 1024 << " KB\n"
            << "  Time:  " << res.duration_sec << "s\n"
            << "  Speed: " << throughput << " MB/s\n";
    }


    static void print_scalability(const TestResult& res) {
        double ops_per_sec = (res.total_bytes / 1024) / res.duration_sec;
        std::cout << "Threads: " << res.threads_count
            << " | Throughput: " << ops_per_sec << " KB/s\n";
    }
};

int main()
{
    CAsyncLogger* pLogger = CAsyncLogger::getInstance();
    if (!pLogger || !pLogger->init())
        return -1;

    LogBenchmark::run_tests();
    return 0;
}

#elif defined(__linux__)

int main_linux()
{
    CAsyncLogger* pLogger = CAsyncLogger::getInstance();
    if (!pLogger || !pLogger->init())
        return -1;

    int i = 0;
    std::chrono::microseconds duration(500);
    LogQueryStu queryStu;
    while (i <= 1000)
    {
        LOGINFO("Message: %s, count: %d", "aaa", i++);
        if (i == 50)
        {
#if defined(_WIN32) || defined(_WIN64)
            SYSTEMTIME st;
            GetLocalTime(&st);
            SystemTimeToFileTime(&st, &queryStu.logStartTime);
#elif defined(__linux__)
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            double dStartTime = static_cast<double>(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
            queryStu.logStartTime = dStartTime;
#endif
        }
        std::this_thread::sleep_for(duration);
    }

    std::function<void(QueryStatus status, const std::string&)> LogCallBack = queryCallBack;
    CLogPreview logPreview;
    queryStu.logLevel = EI_LOG_INFO;
#if defined(_WIN32) || defined(_WIN64)
    SYSTEMTIME st;
    GetLocalTime(&st);
    SystemTimeToFileTime(&st, &queryStu.logEndTime);
#elif defined(__linux__)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    double dEndTime = static_cast<double>(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    queryStu.logEndTime = dEndTime;
#endif	
    logPreview.queryLog(queryStu, LogCallBack);
    getchar();
    return 0;
}

#endif