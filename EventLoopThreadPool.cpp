#include "EventLoopThreadPool.h"

#include "EventLoop.h"
#include "EventLoopThread.h"

#include <assert.h>

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, int numThreads)
    : baseLoop_(baseLoop), numThreads_(numThreads), start_(false), next_(0)
{
    if (numThreads <= 0)
    {
        // LOG
        abort();
    }
}

EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::start()
{
    assert(!start_);
    baseLoop_->assertInLoopThread();
    start_ = true;
    for (int i = 0; i < numThreads_; ++i)
    {
        std::unique_ptr<EventLoopThread> t(new EventLoopThread()); //创建 EventLoopThread 对象
        threads_.push_back(std::move(t));                          // threads_ 内存储所有 EventLoopThread 对象的智能指针，实现自动销毁
        eventLoops_.push_back(t->startLoop());                     //在 EventLoopThread 对象内创建每个线程，并各自运行线程函数
    }
}

EventLoop *EventLoopThreadPool::getNextLoop()
{
    assert(start_);
    baseLoop_->assertInLoopThread();
    EventLoop *eventLoop = baseLoop_;
    if (!eventLoops_.empty())
    {
        eventLoop = eventLoops_[next_]; //指向下一个事件循环
        next_ = (next_ + 1) % numThreads_;
    }
    return eventLoop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops()
{
    assert(start_);
    baseLoop_->assertInLoopThread();
    if (!eventLoops_.empty())
    {
        return eventLoops_;
    }
    else
    {
        return std::vector<EventLoop *>(1, baseLoop_); //若未空 返回主线程
    }
}