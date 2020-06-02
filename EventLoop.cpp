#include "Epoller.h"
#include "EventLoop.h"
#include "CurrentThread.h"
#include "Socket.h"

#include <assert.h>
#include <sys/eventfd.h>

__thread EventLoop *t_loopInThisThread = 0;

EventLoop::EventLoop()
    : looping_(false),
      epoller_(new Epoller()),
      wakeupfd_(createEventfd()),
      quit_(false),
      eventHandling_(false),
      callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()),
      wakeupChannel_(new Channel(this, wakeupfd_))
{
    if (t_loopInThisThread)
    {
        // LOG << "Another EventLoop " << t_loopInThisThread << " exists in this
        // thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }
    // wakeupChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
    wakeupChannel_->setEvents(EPOLLIN | EPOLLET);
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->setModEpollfdEventCallback(std::bind(&EventLoop::modEvent, this, wakeupChannel_, 0));
    epoller_->epoll_add(wakeupChannel_, 0);
}

EventLoop::~EventLoop()
{
    // wakeupChannel_->disableAll();
    // wakeupChannel_->remove();
    close(wakeupfd_);
    t_loopInThisThread = NULL;
}

int EventLoop::createEventfd()
{
    int efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (efd < 0)
    {
        //LOG << "Failed in eventfd";
        abort();
    }
    return efd;
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = Socket::writefd(wakeupfd_, (char *)(&one), sizeof one);
    if (n != sizeof one)
    {
        //LOG << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = Socket::readfd(wakeupfd_, &one, sizeof one);
    if (n != sizeof one)
    {
        //LOG << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
    // wakeupChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
    wakeupChannel_->setEvents(EPOLLIN | EPOLLET);
}

void EventLoop::runInLoop(const std::function<void()> &cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const std::function<void()> &cb)
{
    {
        MutexLockGuard lock(mutex_); //确保跨线程（主线程）调用时的线程安全
        pendingFunctors_.push_back(cb); //存入待定函数数组中
    }

    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup(); //不在此线程调用时唤醒
    }
}

void EventLoop::loop()
{
    assert(!looping_);
    assert(isInLoopThread());
    looping_ = true;
    quit_ = false;
    // LOG_TRACE << "EventLoop " << this << " start looping";
    std::vector<ChannelSP> activeChannels_;
    while (!quit_)
    {
        activeChannels_.clear();
        epoller_->poll(&activeChannels_);
        eventHandling_ = true;
        for (auto &it : activeChannels_)
        {
            it->handleEvents(); //处理活跃事件
        }
        eventHandling_ = false;
        doPendingFunctors(); //处理待定函数（加入新连接）
        epoller_->handleExpired(); //处理超时事件
    }
    looping_ = false;
}

void EventLoop::doPendingFunctors()
{
    std::vector<std::function<void()> > functors;
    callingPendingFunctors_ = true;

    {
        MutexLockGuard lock(mutex_); //确保线程自身调用时的线程安全，防止此时主线程调用queueInLoop，向pendingFunctors_中写
        functors.swap(pendingFunctors_); //将待定函数数组交换出来，减少临界区的操作
    }

    for (const std::function<void()> &functor : functors)
    {
        functor(); //运行待定函数（加入新连接）
    }
    callingPendingFunctors_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup(); //唤醒epoll，从而使loop函数离开while(!quit_)
    }
}