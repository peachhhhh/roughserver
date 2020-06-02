#pragma once

#include "CountDownLatch.h"
#include "noncopyable.h"

#include <pthread.h>
#include <functional>
#include <iostream>
//using namespace std;

class Thread : noncopyable
{
public:
    typedef std::function<void()> ThreadFunc;
    explicit Thread(const ThreadFunc &, const std::string &n = std::string());
    ~Thread();
    void start();
    int join();
    bool isStart() const { return start_; }
    pid_t tid() const { return tid_; }
    const std::string &name() const { return name_; }

private:
    bool start_;           //子线程是否开始
    bool join_;            //子线程是否执行结束
    pthread_t pthreadId_;  //子线程ID
    pid_t tid_;            //子线程pid(进程标识符)
    ThreadFunc func_;      //子线程的执行函数
    std::string name_;     //子线程名字
    CountDownLatch latch_; //倒计时门栓
};