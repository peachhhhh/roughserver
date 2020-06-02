#pragma once

#include "MutexLock.h"
#include "noncopyable.h"

#include <pthread.h>

class Condition : noncopyable
{
public:
    explicit Condition(MutexLock &mutex) : mutex_(mutex)
    {
        pthread_cond_init(&cond_, NULL);
    }
    ~Condition()
    {
        pthread_cond_destroy(&cond_);
    }

    //条件等待
    void wait()
    {
        pthread_cond_wait(&cond_, mutex_.getPthreaadMutex());
    }

    //计时等待，时间范围内条件没有满足，返回false
    bool waitTime(int seconds);

    //唤醒一个阻塞线程
    void notify()
    {
        pthread_cond_signal(&cond_);
    }

    //唤醒全部阻塞线程
    void notifyAll()
    {
        pthread_cond_broadcast(&cond_);
    }

private:
    MutexLock &mutex_;
    pthread_cond_t cond_;
};