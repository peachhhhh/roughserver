#include "Channel.h"
#include "HTTPData.h"

#include <assert.h>
#include <sys/epoll.h>

Channel::Channel(EventLoop *loop, int fd)
    : eventLoop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      eventHandling_(false)
{
}

Channel::~Channel()
{
    assert(!eventHandling_);
}

std::shared_ptr<HTTPData> Channel::getHTTPData()
{
    std::shared_ptr<HTTPData> httpdataSP(httpdata_.lock());
    return httpdataSP;
}

void Channel::handleEvents()
{
    eventHandling_ = true;
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        //事件不能读且被挂起
        //LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";
    }
    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
        {
            errorCallback_(); // 事件出错
        }
    }
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) // 可读 | 带外数据 | 对端正常断开连接
    {
        if (readCallback_)
        {
            readCallback_();
        }
    }
    if (revents_ & EPOLLOUT) // 可写
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
    modEpollfdEvent();
    eventHandling_ = false;
}

void Channel::modEpollfdEvent()
{
    if (modEpollfdEventCallback_)
    {
        modEpollfdEventCallback_();
    }
}