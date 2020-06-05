#include "Thread.h"
#include "CurrentThread.h"

#include <stdio.h>       //snprintf()
#include <unistd.h>      //syscall()
#include <sys/syscall.h> //SYS_gettid
#include <sys/types.h>   //pid_t
#include <sys/prctl.h>   //prctl()
#include <assert.h>
#include <iostream>

namespace CurrentThread
{
    __thread int t_cachedTid = 0;  //线程tid
    __thread char t_tidString[32]; //线程tid字符串
    __thread int t_tidStringLength = 6;
    __thread const char *t_threadName = "default"; //线程名

    /*
    pid_t gettid()
    {
        return static_cast<pid_t>(::syscall(SYS_gettid));
    }
    */

    void cacheTid()
    {
        if (t_cachedTid == 0)
        {
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
            t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
        }
    }

    class ThreadNameInitializer
    {
    public:
        ThreadNameInitializer() // 未考虑fork()
        {
            CurrentThread::t_threadName = "main";
            CurrentThread::tid();
        }
    };

    ThreadNameInitializer init; //用于主进程初始化命名空间CurrentThread内的各个变量，先于main函数初始化

    struct ThreadData
    {
        typedef Thread::ThreadFunc ThreadFunc;
        ThreadFunc func_;
        std::string name_;
        pid_t *tid_;
        CountDownLatch *latch_;

        ThreadData(const ThreadFunc &func, std::string &name, pid_t *tid, CountDownLatch *latch)
            : func_(func), name_(name), tid_(tid), latch_(latch) {}

        void runInThread()
        {
            *tid_ = CurrentThread::tid(); //使得主线程的Thread对象内的tid_存储子线程的tid
            tid_ = NULL;
            latch_->countDown(); //使得主线程的Thread对象内的倒计时门栓latch_减一
            latch_ = NULL;

            CurrentThread::t_threadName = name_.empty() ? "Thread" : name_.c_str();
            prctl(PR_SET_NAME, CurrentThread::t_threadName);

            func_();
            CurrentThread::t_threadName = "finished";
        }
    };

    void *startThread(void *obj)
    {
        ThreadData *data = static_cast<ThreadData *>(obj);
        data->runInThread();
        delete data;
        return NULL;
    }

} // namespace CurrentThread

Thread::Thread(const ThreadFunc &func, const std::string &n)
    : start_(false),
      join_(false),
      pthreadId_(0),
      tid_(0),
      func_(func),
      name_(n),
      latch_(1)
{
    if (name_.empty())
    {
        char buf[32];
        snprintf(buf, sizeof buf, "Thread");
        name_ = buf;
    }
}

Thread::~Thread()
{
    if (start_ && !join_)
    {
        pthread_detach(pthreadId_); //如果析构时，子线程已经开始，且执行结束未join，则分离子线程
    }
}

void Thread::start()
{
    assert(!start_);
    start_ = true;
    CurrentThread::ThreadData *data = new CurrentThread::ThreadData(func_, name_, &tid_, &latch_);
    if (pthread_create(&pthreadId_, NULL, &CurrentThread::startThread, data)) //成功返回0，错误返回错误号
    {
        start_ = false;
        delete data;
        //LOG_SYSFATAL << "Failed in pthread_create";
    }
    else
    {
        latch_.wait();
        assert(tid_ > 0); //判断子线程的tid是否大于0，是否创建成功
    }
}

int Thread::join()
{
    assert(start_); //确定子线程已经开始
    assert(!join_); //确定子线程已经执行结束且未join
    join_ = true;
    return pthread_join(pthreadId_, NULL); //成功返回0，错误返回错误号
}