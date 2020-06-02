#pragma once

#include "noncopyable.h"
#include "Timer.h"
#include "Channel.h"

#include <map>
#include <vector>
#include <memory>
//#include <unordered_map>

class HTTPData;

class Epoller : noncopyable
{
public:
    typedef std::vector<ChannelSP> ChannelSPVector;
    Epoller();
    ~Epoller();

    void epoll_add(ChannelSP channel, int timeout);
    void epoll_mod(ChannelSP channel, int timeout);
    void epoll_del(ChannelSP channel);

    int poll(ChannelSPVector *activeChannels);
    void getActiveChannels(int numEvents, ChannelSPVector* activeChannels);

    void setTimer(ChannelSP channel, int timeout);
    int getEpollfd() const { return epollfd_; }
    void handleExpired();

private:
    //static const int MAXFDS = 100000;
    static const int eventsSize = 4096;
    static const int epollTimeout = 10000;
    int epollfd_;
    std::vector<epoll_event> events_;

    typedef std::map<int, ChannelSP> ChannelSPMap;
    ChannelSPMap channels_;
    //std::shared_ptr<Channel> fd2chan_[MAXFDS];

    typedef std::map<int, std::shared_ptr<HTTPData> > HTTPDataSPMap;
    HTTPDataSPMap httpdatas_;
    //std::shared_ptr<HTTPData> fd2http_[MAXFDS];

    TimerManager timerManager_;
};