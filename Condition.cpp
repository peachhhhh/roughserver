#include "Condition.h"

#include <time.h>  //CLOCK_REALTIME  timespec  clock_gettime()
#include <errno.h> //ETIMEDOUT
#include <pthread.h>


//计时等待，时间范围内条件没有满足，返回false
bool Condition::waitTime(int seconds)
{
    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += static_cast<time_t>(seconds);
    return ETIMEDOUT == pthread_cond_timedwait(&cond_, mutex_.getPthreaadMutex(), &abstime);
}