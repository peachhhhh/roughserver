#pragma once

#include <memory>

class Timer;
class Channel;
class EventLoop;

enum ProcessState
{
    STATE_PARSE_URI = 1,
    STATE_PARSE_HEADERS,
    STATE_RECV_BODY,
    STATE_ANALYSIS,
    STATE_FINISH
};

enum URIState
{
    PARSE_URI_AGAIN = 1,
    PARSE_URI_ERROR,
    PARSE_URI_SUCCESS,
};

enum HeaderState
{
    PARSE_HEADER_SUCCESS = 1,
    PARSE_HEADER_AGAIN,
    PARSE_HEADER_ERROR
};

enum AnalysisState
{
    ANALYSIS_SUCCESS = 1,
    ANALYSIS_ERROR
};

enum ParseState
{
    H_START = 0,
    H_KEY,
    H_COLON,
    H_SPACES_AFTER_COLON,
    H_VALUE,
    H_CR,
    H_LF,
    H_END_CR,
    H_END_LF
};

enum ConnectionState
{
    H_CONNECTED = 0,
    H_DISCONNECTING,
    H_DISCONNECTED
};

enum HttpMethod
{
    METHOD_POST = 1,
    METHOD_GET,
    METHOD_HEAD
};

enum HttpVersion
{
    HTTP_10 = 1,
    HTTP_11
};

class HTTPData
{
public:
    HTTPData(EventLoop *eventLoop, int fd);
    ~HTTPData();
    void newEvent();
    void closeConn();

    std::shared_ptr<Channel> getChannel() { return channel_; }
    EventLoop *getEventLoop() { return eventLoop_; }
    void setTimer(std::shared_ptr<Timer> timer) { timer_ = timer; }

private:
    void handleRead();
    void handleWrite();
    void modEpollfdEventCallback();
    //void handleError(int fd, int err_num, std::string short_msg);

    EventLoop *eventLoop_;
    int fd_;
    std::shared_ptr<Channel> channel_;
    std::weak_ptr<Timer> timer_; //weak_ptr，只观测Timer
    ConnectionState connectionState_;

    const int expiredTime = 2000;
};