#pragma once

#include <functional>
#include <memory>
#include <sys/epoll.h>

class EventLoop;
class HTTPData;

//事件分发
//每个Channel对象都只属于某一个IO线程(一个IO线程可以有若干个Channel对象)，操作时不用加锁
//只负责一个文件描述符的IO事件分发，但不拥有它，不负责关闭
//生命周期由所属类管理（wakeupchannel由EventLoop管理，acceptChannel由Server管理，其余的一般channel由HTTPData管理）
class Channel
{
public:
    typedef std::function<void()> EventCallback;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    void setReadCallback(const EventCallback &cb) { readCallback_ = cb; }
    void setWriteCallback(const EventCallback &cb) { writeCallback_ = cb; }
    void setCloseCallback(const EventCallback &cb) { closeCallback_ = cb; }
    void setErrorCallback(const EventCallback &cb) { errorCallback_ = cb; }

    void setModEpollfdEventCallback(const EventCallback &cb) { modEpollfdEventCallback_ = cb; }

    void setEvents(int events) { events_ = events; }
    void setRevents(int revents) { revents_ = revents; }
    void setHTTPData(std::shared_ptr<HTTPData> httpdata) { httpdata_ = httpdata; }

    int getfd() { return fd_; }
    unsigned int &getEvents() { return events_; }
    std::shared_ptr<HTTPData> getHTTPData();

    void handleEvents();

    //void handleError();
    void modEpollfdEvent(); // 更新epollfd中的事件

private:
    EventLoop *eventLoop_; //所属的EventLoop对象
    const int fd_;         //文件描述符
    unsigned int events_;           //关注的事件
    unsigned int revents_;          //epoll返回的活动事件
    //int index_;            //epoll的事件数组中的序号
    bool eventHandling_;

    std::weak_ptr<HTTPData> httpdata_; //httpdata生命周期不由channel控制，因此使用weak_ptr

    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
    EventCallback modEpollfdEventCallback_;
};

typedef std::shared_ptr<Channel> ChannelSP;