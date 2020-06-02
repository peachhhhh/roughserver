#pragma once

#include "noncopyable.h"

#include <queue>
#include <deque>
#include <memory>
#include <unistd.h>
#include <sys/time.h>

//只被所属的IO线程调用，不用加锁

class HTTPData;

class Timer
{
public:
    Timer(std::shared_ptr<HTTPData> httpdata, int timeout);
    ~Timer();
    //Timer(Timer &timer);
    //void update(int timeout);
    bool isUnexpired();
    void clearHTTP();

    void setDeleted() { deleted_ = true; }
    bool isDeleted() const { return deleted_; }
    size_t getExpTime() const { return expiredTime_; }

private:
    inline size_t timeConvert(struct timeval time)
    {
        return (((time.tv_sec % 10000) * 1000) + (time.tv_usec / 1000));
    }

    bool deleted_;
    std::shared_ptr<HTTPData> httpdata_;
    size_t expiredTime_;
};

//仿函数，用于小根堆
struct TimerCmp
{
    bool operator()(std::shared_ptr<Timer> &a, std::shared_ptr<Timer> &b) const
    {
        return a->getExpTime() > b->getExpTime();
    }
};

class TimerManager : noncopyable
{
public:
    TimerManager();
    ~TimerManager();
    void addTimer(std::shared_ptr<HTTPData> httpdata, int timeout);
    void handleExpiredEvent();

private:
    typedef std::shared_ptr<Timer> TimerSP;
    std::priority_queue<TimerSP, std::deque<TimerSP>, TimerCmp> timerQueue; //小根堆
};