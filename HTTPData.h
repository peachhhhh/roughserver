#pragma once

#include <memory>

class Timer;
class Channel;
class EventLoop;

class HTTPData
{
public:
    HTTPData(EventLoop *eventLoop, int fd);
    void newEvent();
    void handleClose();

    std::shared_ptr<Channel> getChannel() { return channel_; }
    void setTimer(std::shared_ptr<Timer> timer) { timer_ = timer; }

private:
    std::shared_ptr<Channel> channel_;
    std::weak_ptr<Timer> timer_; //weak_ptr，只观测Timer
};