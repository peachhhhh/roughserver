#pragma once

#include "noncopyable.h"

#include <map>
#include <string.h>
#include <memory>

class Channel;
class EventLoop;
class EventLoopThreadPool;

class Server : noncopyable
{
public:
    Server(EventLoop *baseLoop, int numThread, int port);
    ~Server();
    EventLoop *getBaseLoop() const { return baseLoop_; }
    void start();
    void acceptNewConn();
    void handleCurConn() { baseLoop_->modChannel(acceptChannel_); }

private:
    bool start_;
    EventLoop *baseLoop_;
    int numThread_;
    int port_;
    std::unique_ptr<EventLoopThreadPool> eventLoopThreadPool_;

    int listenfd_;
    std::shared_ptr<Channel> acceptChannel_;
    //static const int MAXFDS = 100000;
};