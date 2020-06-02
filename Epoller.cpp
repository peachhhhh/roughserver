#include "Epoller.h"

#include <assert.h>
#include <sys/epoll.h>

Epoller::Epoller() : epollfd_(epoll_create1(EPOLL_CLOEXEC)), events_(eventsSize) //EPOLL_CLOEXEC：fork后关闭描述符
{
    assert(epollfd_ > 0);
}

Epoller::~Epoller()
{
    close(epollfd_);
}

void Epoller::epoll_add(ChannelSP channel, int timeout)
{
    int fd = channel->getfd();
    struct epoll_event event;
    event.data.fd = fd;
    event.events = channel->getEvents();
    //channel->EqualAndUpdateLastEvents();
    channels_[fd] = channel;
    if (timeout > 0)
    {
        setTimer(channel, timeout);
        httpdatas_[fd] = channel->getHTTPData();
    }
    if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        //LOG_SYSFATAL
        channels_[fd].reset();
    }
}

void Epoller::epoll_mod(ChannelSP channel, int timeout)
{
    if (timeout > 0)
    {
        setTimer(channel, timeout);
    }
    int fd = channel->getfd();
    //if (!channel->EqualAndUpdateLastEvents())
    //{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = channel->getEvents();
    if (epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &event) < 0)
    {
        //LOG_SYSFATAL
        channels_[fd].reset();
    }
    //}
}

void Epoller::epoll_del(ChannelSP channel)
{
    int fd = channel->getfd();
    struct epoll_event event;
    event.data.fd = fd;
    event.events = channel->getEvents();
    // event.events = 0;
    // request->EqualAndUpdateLastEvents()
    if (epoll_ctl(epollfd_, EPOLL_CTL_DEL, fd, &event) < 0)
    {
        //LOG_SYSFATAL  perror("epoll_del error");
    }
    channels_[fd].reset();
    httpdatas_[fd].reset();
}

int Epoller::poll(ChannelSPVector *activeChannels)
{
    //LOG_TRACE
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), epollTimeout);
    if (numEvents > 0)
    {
        //LOG_TRACE
        getActiveChannels(numEvents, activeChannels);
        if (numEvents == static_cast<int>(events_.size()))
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        //LOG_TRACE
    }
    else
    {
        //LOG_SYSERR
    }
    return numEvents;
}

void Epoller::getActiveChannels(int numEvents, ChannelSPVector *activeChannels)
{
    for (int i = 0; i < numEvents; ++i)
    {
        int fd = events_[i].data.fd; //活跃事件描述符
        if (channels_[fd])
        {
            channels_[fd]->setRevents(events_[i].events);
            // channels_[fd]->setEvents(0);  ?
            // 加入线程池之前将Timer和request分离
            // cur_req->seperateTimer();
            activeChannels->push_back(channels_[fd]);
        }
        else
        {
            //LOG
        }
    }
}

void Epoller::handleExpired()
{
    timerManager_.handleExpiredEvent();
}

void Epoller::setTimer(ChannelSP channel, int timeout)
{
    std::shared_ptr<HTTPData> tmp = channel->getHTTPData();
    if (tmp)
    {
        timerManager_.addTimer(tmp, timeout);
    }
    else
    {
        //LOG << "timer add fail";
    }
}