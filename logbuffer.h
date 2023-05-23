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
    enum BufState { FREE = 0, FLUSH = 1 }; //FREE ����״̬ ��д����־, FLUSH ��д�������д���ļ�    

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
`std::array<char, BUFSIZE>`�еĵڶ�������������Ĵ�С����ʹ��`std::array`��������ʱ�����ṩ��
��������ڹ��캯���ĳ�ʼ���б���ȷ�����С������Ҫ������Ϊģ��������ݣ�������ʾ��

```
template <std::size_t N>
class MyClass {
  std::array<char, N> buffer;
public:
  MyClass() : buffer() {}
};
```
����Ĵ��뽫�����С��Ϊ��ģ��������ݣ����������ڴ������顣
�ڹ��캯���ĳ�ʼ���б��г�ʼ������ʱ��ֻ��Ҫ����Ĭ�Ϲ��캯�����ɡ�
*/

#endif // !LOGBUFFER_H
