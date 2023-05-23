#ifndef LOGBUFFER_H
#define LOGBUFFER_H


#include<array>
#include<cstdio>
#include<iostream>
#include<cstring>

const int BUFSIZE       = 8 * 1024 * 1024;//16MB


template <std::size_t N= BUFSIZE>
class LogBuffer {
public:
    enum BufState { FREE = 0, FLUSH = 1 }; //FREE 空闲状态 可写入日志, FLUSH 待写入或正在写入文件    

    LogBuffer() :buffer{ 0 },bufSize(N), usedSize(0), state(BufState::FREE) {};

    ~LogBuffer(){}

    inline int getAvailableSize()   const { return bufSize - usedSize; }
    inline int getState()           const { return state; }
    inline int getBufferSize()      const { return bufSize; }
#ifdef DEBUG_
    inline void debugPrint() { std::cout << buffer.data();}
#endif // DEBUG

    void append(const char* logline, int len);
    void flushToFile(FILE* fp);
private:
    std::array<char,N>  buffer;
    size_t              bufSize;
    size_t              usedSize;
    int                 state;

};

template <std::size_t N >
void LogBuffer<N>::append(const char* logline, int len) {
    memcpy( buffer.data() + usedSize , logline, len);
    usedSize += len;
}

template <std::size_t N >
void LogBuffer<N>::flushToFile(FILE* fp) {
    if (fp) {
        size_t wt_len = fwrite(buffer.data(), sizeof(char), usedSize, fp);
        if (wt_len != usedSize) {
            std::cerr << "fwrite fail!" << std::endl;
        }
        usedSize = 0;
        fflush(fp);
    }
    else {
        std::cerr << "fd closed" << std::endl;
    }
}

/*
`std::array<char, BUFSIZE>`中的第二个参数是数组的大小，在使用`std::array`创建数组时必须提供。
如果您想在构造函数的初始化列表中确定其大小，则需要将其作为模板参数传递，如下所示：

```
template <std::size_t N>
class MyClass {
  std::array<char, N> buffer;
public:
  MyClass() : buffer() {}
};
```
上面的代码将数组大小作为类模板参数传递，并将其用于创建数组。
在构造函数的初始化列表中初始化数组时，只需要调用默认构造函数即可。
*/

#endif // !LOGBUFFER_H
