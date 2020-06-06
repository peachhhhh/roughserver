#include "Timer.h"
#include "HTTPData.h"

Timer::Timer(std::shared_ptr<HTTPData> requestData, int timeout)
    : deleted_(false), httpdata_(requestData)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    expiredTime_ = timeConvert(now) + timeout;
}

Timer::~Timer()
{
    if (httpdata_)
        httpdata_->closeConn(); //超时后，析构时关闭连接
}

/*
Timer::Timer(Timer &timer)
    : httpdata_(timer.httpdata_), expiredTime_(0)
{
}

void Timer::update(int timeout)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    expiredTime_ = timeConvert(now) + timeout;
}
*/

bool Timer::isUnexpired()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    size_t temp = timeConvert(now);
    if (temp < expiredTime_)
        return true;
    else
    {
        this->setDeleted();
        return false;
    }
}

void Timer::resetHTTPSP()
{
    httpdata_.reset(); //shared_ptr重置
    this->setDeleted();
}

TimerManager::TimerManager() {}

TimerManager::~TimerManager() {}

void TimerManager::addTimer(std::shared_ptr<HTTPData> httpdata_, int timeout)
{
    TimerSP newTimer(new Timer(httpdata_, timeout));
    timerQueue.push(newTimer);
    httpdata_->setTimer(newTimer);
}

void TimerManager::handleExpiredEvent()
{
    while (!timerQueue.empty())
    {
        TimerSP topTimer = timerQueue.top();
        if (topTimer->isDeleted())
            timerQueue.pop();
        else if (topTimer->isUnexpired() == false)
            timerQueue.pop(); //pop后，pop出来的Timer的shared_ptr引用减为0，该Timer析构
        else
            break;
    }
}