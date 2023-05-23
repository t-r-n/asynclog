
#include "asynclog.h"
#include "logbuffer.h"
#include<cstdio>

using namespace std;

AsyncLog* AsyncLog::logger = new AsyncLog;

void singleThread(int nums=100*10000) {
    for (int i = 0; i < nums; ++i) {
        LOG(LogLevel::DEBUG, "hello world :%d \n", i);
    }
    LOG_STOP();
}
void multiThread(int nums=100*10000,int n=4) {
    std::atomic<int> step = { 0 };
    vector<std::thread>ve;
    for (int i = 0; i < n; ++i) {
        ve.push_back(std::thread([&step,&nums]() {
            for (int i = 0; i < nums; ++i) {
                LOG(LogLevel::DEBUG, "hello world :%d \n", i);
            }
            step++;
            }));
        ve[i].detach();
    }
    while (1) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (step.load() == n) {
            LOG_STOP();
            break;
        }
        else {
            std::this_thread::yield();
        }
    }
}

int main()
{

    LOG_INIT("./test.txt", LogLevel::DEBUG);

    int nums = 100 * 10000;

    // 获取起始时间
    auto start_time = std::chrono::high_resolution_clock::now();

    singleThread();//单线程测试

    //multiThread();//多线程测试


    
    // 获取当前时间
    auto end_time = std::chrono::high_resolution_clock::now();
    // 计算时间间隔
    auto time_span = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    // 输出时间间隔（以微秒为单位）
    std::cout << "Time elapsed: " << time_span.count() << " milliseconds" << std::endl;


    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}
