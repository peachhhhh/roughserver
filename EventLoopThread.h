#pragma once

#include "Condition.h"
#include "MutexLock.h"
#include "Thread.h"

class EventLoop;

class EventLoopThread
{
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop *startLoop();

private:
    void threadFunc();//线程函数

    EventLoop* eventLoop_; //事件循环指针
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
};