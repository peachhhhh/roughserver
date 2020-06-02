#pragma once

namespace CurrentThread
{
    extern __thread int t_cachedTid;      //线程tid
    extern __thread char t_tidString[32]; //线程tid字符串
    extern __thread int t_tidStringLength;
    extern __thread const char *t_threadName; //线程名

    void cacheTid(); //获取线程tid，只在t_cachedTid为0时调用

    inline int tid() //返回线程tid，首次使用时，调用cacheTid获取
    {
        //因为t_cachedTid==0是false的概率更大，所以用__builtin_expect使编译器优化代码，减少指令跳转
        if (__builtin_expect(t_cachedTid == 0, 0))
        {
            cacheTid();
        }
        return t_cachedTid;
    }

    inline const char *tidString() //用于logging
    {
        return t_tidString;
    }

    inline int tidStringLength() //用于logging
    {
        return t_tidStringLength;
    }

    inline const char *name()
    {
        return t_threadName;
    }

    class ThreadNameInitializer
    {
    public:
        ThreadNameInitializer();
    };

    ThreadNameInitializer init; //用于进程初始化命名空间CurrentThread内的各个变量

    void *startThread(void *obj); //子线程运行函数
} // namespace CurrentThread