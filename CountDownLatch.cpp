#include "CountDownLatch.h"

CountDownLatch::CountDownLatch(int count)
    : mutex_(), cond_(mutex_), count_(count)
{
}

void CountDownLatch::wait()
{
    MutexLockGuard lock(mutex_);
    while (count_ > 0) //使用while，防止虚假唤醒(原因：pthread_cond_signal可能会激活多于一个线程)
    {
        cond_.wait();
    }
}

void CountDownLatch::countDown()
{
    MutexLockGuard lock(mutex_);
    if (--count_ == 0)
    {
        cond_.notifyAll();
    }
}