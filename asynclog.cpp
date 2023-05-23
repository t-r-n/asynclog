// asyncLog.cpp: 定义应用程序的入口点。
//
#include "asynclog.h"
#include <cstdarg>
#include <ctime>
#include <iomanip>


const char* LevelString[5] = { "DEBUG", "INFO", "WARNING", "ERROR", "FATAL" };



AsyncLog::AsyncLog(/* args */) :
    level(LogLevel::INFO)
{
    currentBuffer = std::make_unique<LogBuffer<>>();
    nextBuffer    = std::make_unique<LogBuffer<>>();
    flushInterval_ = std::chrono::milliseconds(1000);
}

AsyncLog::~AsyncLog()
{

    if (outputFile) {
        fclose(outputFile);
        outputFile = nullptr;
    }

}

void AsyncLog::Init(const char* logdir, LogLevel lev) {


    lastTime = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(lastTime);
    std::tm tim = *std::localtime(&now_time_t);
    int k = snprintf(save_ymdhms, 64, "%04d-%02d-%02d %02d:%02d:%02d", tim.tm_year + 1900, \
        tim.tm_mon + 1, tim.tm_mday, tim.tm_hour, tim.tm_min, tim.tm_sec);
    save_ymdhms[k] = '\0';

    level = lev;
    outputFile = fopen(logdir, "w+");
    if (outputFile == nullptr) {
        printf("logfile open fail!\n");
    }

    //create flush thread
    std::thread(&AsyncLog::writeToFile, this).detach();
    return;
}

void AsyncLog::Append(int level, const char* file, int line, const char* func, const char* fmt, ...) {
    if (!running_.load())return;

    //单行日志
    char logLine[LOGLINESIZE];

    //性能优化 1.秒数不变则不调用localtime.
    //2.并且继续复用之前的年月日时分秒的字符串，减少snprintf中太多参数格式化的开销

    auto now_time = std::chrono::system_clock::now();
    // 计算时间间隔
    auto time_span = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - lastTime);

    auto us = std::chrono::duration_cast<std::chrono::microseconds>(now_time.time_since_epoch()) % 1000000;
    
    if (time_span.count() >= 1000) {//间隔大于一秒重新更改时间字符串
        auto now_time_t = std::chrono::system_clock::to_time_t(now_time);
        std::tm tim = *std::localtime(&now_time_t);
        int k = snprintf(save_ymdhms, 64, "%04d-%02d-%02d %02d:%02d:%02d", tim.tm_year + 1900, \
                tim.tm_mon + 1, tim.tm_mday, tim.tm_hour, tim.tm_min, tim.tm_sec);
        save_ymdhms[k] = '\0';
        lastTime = now_time;
    }

    std::thread::id tid = std::this_thread::get_id();
    uint32_t n = snprintf(logLine, LOGLINESIZE, "[%s][%s.%06d][%s:%d %s][pid:%u] ", LevelString[level], \
        save_ymdhms,us, file, line, func, std::hash<std::thread::id>()(tid));

    va_list args;
    va_start(args, fmt);
    int m = vsnprintf(logLine + n, LOGLINESIZE - n, fmt, args);
    va_end(args);


    int length = n + m;





#if 1
    //将消息压入缓冲区
    //这部分应该异步吗？
    std::unique_lock<std::mutex> bufLock(bufMutex);
    bufMutex_cv.wait(bufLock, [this]() {return ready; });//多线程共用一个currentBuffer
    //可优化为多线程各有一个buffer
    if (!running_.load())return;
    ready = false;
    if (currentBuffer->getAvailableSize() > length) {
        currentBuffer->append(logLine, length);
    }
    else {
        bufferQueue.push(std::move(currentBuffer));

        if (nextBuffer) {
            //std::cout << "nextBuffer is Ready" << std::endl;
            currentBuffer = std::move(nextBuffer);
        }
        else {//如果nextBuffer没有准备好,缓冲区压力过大
            //std::cout << "nextBuffer isUnReady" << std::endl;
            currentBuffer.reset(new LogBuffer<>);
        }
        currentBuffer->append(logLine, length);
    }
    ready = true;
    bufMutex_cv.notify_one();
#endif
}
void AsyncLog::writeToFile() {
    std::unique_ptr<LogBuffer<>> newBuffer1(new LogBuffer<>);
    std::unique_ptr<LogBuffer<>> newBuffer2(new LogBuffer<>);
    std::queue < std::unique_ptr<LogBuffer<>>> bufferQueueToWrite;
    while (running_) {
        {
            std::unique_lock<std::mutex> buffers_lock(bufMutex);
            
            if (bufferQueue.empty()) {
                buffers_lock.unlock();
                std::this_thread::sleep_for(flushInterval_);
                buffers_lock.lock();
            }

            bufferQueue.push(std::move(currentBuffer));
            currentBuffer = std::move(newBuffer1);
            bufferQueueToWrite.swap(bufferQueue);
            if (!nextBuffer) {
                nextBuffer = std::move(newBuffer2);
            }
        }
        //写入文件
        while (!bufferQueueToWrite.empty()) {
            if (outputFile) {
                bufferQueueToWrite.front()->flushToFile(outputFile);
                if (!newBuffer2) {//buffer重用
                    newBuffer2 = std::move(bufferQueueToWrite.front());
                    bufferQueueToWrite.pop();
                    continue;
                }
                if (!newBuffer1) {//buffer重用
                    newBuffer1 = std::move(bufferQueueToWrite.front());
                    bufferQueueToWrite.pop();
                    continue;
                }

                bufferQueueToWrite.pop();
            }
        }

        //重新找缓冲区
        if (!newBuffer1) {
            newBuffer1 = std::make_unique<LogBuffer<>>();
        }
        if (!newBuffer2) {
            newBuffer2 = std::make_unique<LogBuffer<>>();
        }
    }

    //last flush
    std::lock_guard<std::mutex>lck(bufMutex);
    if (currentBuffer)bufferQueue.push(std::move(currentBuffer));
    while (!bufferQueueToWrite.empty()) {
        if (outputFile) {
            bufferQueueToWrite.front()->flushToFile(outputFile);
            bufferQueueToWrite.pop();
        }
    }
    while (!bufferQueue.empty()) {
        if (outputFile) {
            bufferQueue.front()->flushToFile(outputFile);
            bufferQueue.pop();
        }
    }
    //std::cout << "flush all log" << std::endl;
}
void AsyncLog::flush() {
    std::lock_guard<std::mutex>lck(bufMutex);
    if (outputFile) {
        if (currentBuffer)bufferQueue.push(std::move(currentBuffer));
        while (!bufferQueue.empty()) {
            bufferQueue.front()->flushToFile(outputFile);
            bufferQueue.pop();
        }
    }
}
void AsyncLog::stop() { 
    this->running_.store(false); 
};




