#pragma once
#include "Condition.h"
#include "MutexLock.h"
#include "noncopyable.h"

//倒计时门栓
//作用1：在主线程创建一个或多个子线程后，等待子线程完成初始化，主线程再继续执行
//作用2：在主线程创建一个或多个子线程后，子线程等待主线程完成一些任务，子线程再继续执行
class CountDownLatch : noncopyable
{
public:
    CountDownLatch(int count);
    void wait();
    void countDown();

private:
    MutexLock mutex_; //若使用mutable，则可被const成员函数访问
    Condition cond_;
    int count_;
};