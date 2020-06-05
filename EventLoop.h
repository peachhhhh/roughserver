#pragma once

#include "noncopyable.h"
#include "MutexLock.h"
#include "Epoller.h"
#include "Channel.h"
#include "CurrentThread.h"

#include <assert.h>
#include <sys/socket.h>
#include <vector>
#include <memory>
#include <functional>

class EventLoop : noncopyable
{
public:
    EventLoop();
    ~EventLoop();
    void assertInLoopThread() { assert(isInLoopThread()); }
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    void loop();
    void quit();
    void runInLoop(const std::function<void()> &cb);
    void queueInLoop(const std::function<void()> &cb);

    void addfd(ChannelSP channel, int timeout = 0) { epoller_->epoll_add(channel, timeout); }
    void modfdEvent(ChannelSP channel, int timeout = 0) { epoller_->epoll_mod(channel, timeout); }
    void delfd(ChannelSP channel) { epoller_->epoll_del(channel); }

    void shutDown(ChannelSP channel) { shutdown(channel->getfd(), SHUT_WR); } //shutdown关闭连接，如果输出缓冲区中还有未传输的数据，则将传递到目标主机。

private:
    void wakeup();
    void handleRead(); //eventfd wake up
    void doPendingFunctors();
    int createEventfd();

    bool looping_;
    std::shared_ptr<Epoller> epoller_;
    int wakeupfd_; //eventfd，防止在epoll阻塞时候，新连接到来却无法唤醒
    bool quit_;
    bool eventHandling_;
    mutable MutexLock mutex_;
    std::vector<std::function<void()>> pendingFunctors_;
    bool callingPendingFunctors_;
    const pid_t threadId_;
    ChannelSP wakeupChannel_;
};