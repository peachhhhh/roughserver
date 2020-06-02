#include "EventLoopThread.h"

#include "EventLoop.h"

#include <assert.h>

//线程函数利用std::bind函数适配器，绑定上当前对象指针this
EventLoopThread::EventLoopThread()
    : eventLoop_(NULL),
      thread_(std::bind(&EventLoopThread::threadFunc, this), "EventLoopThread"),
      mutex_(),
      cond_(mutex_)
{
}

EventLoopThread::~EventLoopThread()
{
    if (eventLoop_ != NULL)
    {
        eventLoop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop()
{
    assert(!thread_.isStart());
    thread_.start(); //创建一个线程，线程内运行了 threadFunc 线程函数

    {
        MutexLockGuard lock(mutex_);
        while (eventLoop_ == NULL)
        {
            cond_.wait(); //使用条件变量 等待线程函数中的 eventLoop_ = &loop;
        }
    }

    return eventLoop_; //安全返回当前线程的事件循环指针
}

void EventLoopThread::threadFunc()
{
    EventLoop loop; //线程函数中创建EventLoop对象

    {
        MutexLockGuard lock(mutex_);
        eventLoop_ = &loop;
        cond_.notify();
    }

    loop.loop(); //并调用loop()
    MutexLockGuard lock(mutex_);
    eventLoop_ = NULL;
}