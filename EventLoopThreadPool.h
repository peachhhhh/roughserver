#pragma once

#include "noncopyable.h"

#include <vector>
#include <memory>
#include <functional>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    typedef std::function<void(EventLoop *)> ThreadInitCallback;

    EventLoopThreadPool(EventLoop *baseLoop, int numThreads);
    ~EventLoopThreadPool();

    void start();
    EventLoop *getNextLoop();
    std::vector<EventLoop *> getAllLoops();
    bool start() const
    {
        return start_;
    }
    /*
    const std::string &name() const
    {
        return name_;
    }
    */

private:
    EventLoop *baseLoop_; //服务器主线程
    int numThreads_;
    //std::string name_;
    int start_;                                             //是否已经创建线程池的子线程
    int next_;                                              //下一个事件的索引
    std::vector<std::unique_ptr<EventLoopThread>> threads_; //子线程的智能指针数组
    std::vector<EventLoop *> eventLoops_;                   //事件循环的数组
};