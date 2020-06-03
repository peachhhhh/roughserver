#include "HTTPData.h"
#include "Channel.h"
#include "EventLoop.h"

HTTPData::HTTPData(EventLoop *eventLoop, int fd)
    : eventLoop_(eventLoop),
      fd_(fd),
      channel_(new Channel(eventLoop, fd)),
      connectionState_(H_CONNECTED)
{
    channel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT); // EPOLLONESHOT，只触发一次，触发以后，要重新加入epoll，从而保证只在一个线程中触发
    channel_->setReadCallback(std::bind(&HTTPData::handleRead, this));
    channel_->setWriteCallback(std::bind(&HTTPData::handleWrite, this));
    channel_->setModEpollfdEventCallback(std::bind(&HTTPData::modEpollfdEventCallback, this));
}

HTTPData::~HTTPData()
{
    close(fd_);
}

void HTTPData::newEvent()
{
    channel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
    eventLoop_->addfd(channel_, expiredTime); // EPOLLONESHOT，只触发一次，触发以后，要重新加入epoll，从而保证只在一个线程中触发
}

void HTTPData::closeConn()
{
    connectionState_ = H_DISCONNECTED;
    eventLoop_->delfd(channel_);
}