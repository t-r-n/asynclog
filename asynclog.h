// asyncLog.h: 标准系统包含文件的包含文件
// 或项目特定的包含文件。

#ifndef ASYNCLOG_H
#define ASYNCLOG_H

#include <iostream>
#include <thread>

#include <mutex>
#include <condition_variable>
#include <map>
#include <queue>
#include"logbuffer.h"



const int LOGLINESIZE = 1024; //1KB
const int MEM_LIMIT = 512 * 1024 * 1024; //512MB

#define LOG_INIT(logdir, lev) \
    do { \
        AsyncLog::GetInstance()->Init(logdir, lev); \
    } while (0)

#define LOG(lev, fmt, ...) \
    do {  \
        if(AsyncLog::GetInstance()->GetLevel() <= lev) {  \
            AsyncLog::GetInstance()->Append(lev, __FILE__, __LINE__, __FUNCTION__, fmt, __VA_ARGS__);  \
        }  \
    } while (0)  
 
#define LOG_STOP() \
    do { \
        AsyncLog::GetInstance()->stop(); \
    } while (0)



enum LogLevel {
    DEBUG = 0,
    INFO,
    WARING,
    ERROR,
    FATAL
};

class AsyncLog
{
public:

    ~AsyncLog();
    //单例模式
    static AsyncLog* GetInstance() {
        return logger;
    }

    //初始化
    void Init(const char* logdir, LogLevel lev);

    //获取日志等级
    int GetLevel() const {
        return level;
    }

    //写日志__FILE__, __LINE__, __func__,
    void Append(int level, const char* file, int line, const char* func, const char* fmt, ...);

    //flush func
    void flush();//立刻刷新到文件

    void stop(); 
private:
    //单例模式
    static AsyncLog* logger;
    AsyncLog(/* args */);

    //日志等级
    int level;

    //save_ymdhms数组，保存年月日时分秒以便复用
    char save_ymdhms[64];


    std::atomic<bool> running_ = { true };
    std::mutex bufMutex;
    std::condition_variable bufMutex_cv;
    bool ready = true;
    std::queue < std::unique_ptr<LogBuffer<>>> bufferQueue;//buffqueue里存当前所有的buffer，buffer里的状态记录是否可成为nextbuffer
    std::unique_ptr<LogBuffer<>> currentBuffer;
    std::unique_ptr<LogBuffer<>> nextBuffer;
    FILE* outputFile;
    decltype(std::chrono::milliseconds(1000)) flushInterval_;
    decltype(std::chrono::system_clock::now()) lastTime;
    void writeToFile();

};


#endif // !ASYNCLOG_H



