#pragma once

#include <map>
#include <memory>

class Timer;
class Channel;
class EventLoop;

enum GeneralState
{
    PARSE_REQUEST_LINE,
    PARSE_HEADERS,
    RECV_BODY,
    ANALYSIS,
    FINISH
}; //总的状态

enum RequestLineState
{
    PARSE_REQUESTLINE_AGAIN,
    PARSE_REQUESTLINE_ERROR,
    PARSE_REQUESTLINE_SUCCESS
}; //分析请求行的状态

enum HeaderState
{
    PARSE_HEADER_AGAIN,
    PARSE_HEADER_ERROR,
    PARSE_HEADER_SUCCESS
}; //分析请求头的状态

enum HeaderInternalState
{
    START,
    KEY,
    COLON,
    SPACES_AFTER_COLON,
    VALUE,
    CR,
    LF,
    END_CR,
    END_LF
}; //分析请求头内部的状态

enum AnalysisState
{
    ANALYSIS_SUCCESS,
    ANALYSIS_ERROR
};

enum ConnectionState
{
    CONNECTED,     //连接的
    DISCONNECTING, //正在断开
    DISCONNECTED   //已经断开
};

enum HTTPMethod
{
    UnknownMethod,
    GET,
    POST,
    HEAD
};

enum HTTPVersion
{
    UnknownVersion,
    HTTP10,
    HTTP11
};

const static std::map<std::string, std::string> mimetype = {
    {"default", "text/html"},
    {".avi", "video/x-msvideo"},
    {".bmp", "image/bmp"},
    {".c", "text/plain"},
    {".cpp", "text/plain"},
    {".doc", "application/msword"},
    {".gif", "image/gif"},
    {".gz", "application/x-gzip"},
    {".htm", "text/html"},
    {".html", "text/html"},
    {".ico", "image/x-icon"},
    {".jpg", "image/jpeg"},
    {".mp3", "audio/mp3"},
    {".png", "image/png"},
    {".txt", "text/plain"}};

class HTTPData : public std::enable_shared_from_this<HTTPData>
{
public:
    HTTPData(EventLoop *eventLoop, int fd);
    ~HTTPData();
    void newEvent();
    void closeConn();
    void reset();
    void resetTimer();

    std::shared_ptr<Channel> getChannel() { return channel_; }
    EventLoop *getEventLoop() { return eventLoop_; }
    void setTimer(std::shared_ptr<Timer> timer) { timer_ = timer; }

    void handleError(int fd, int err_num, std::string short_msg);

private:
    void handleRead();
    void handleWrite();
    void modEpollfdEventCallback();
    RequestLineState parseRequestLine(); //分析请求行
    HeaderState parseHeaders();          //分析请求头
    AnalysisState handleRequest();

    EventLoop *eventLoop_;
    int fd_;
    bool error_;
    bool keepAlive_;
    std::string fileName_;
    std::string inBuffer_;
    std::string outBuffer_;
    std::map<std::string, std::string> headers_; //请求头信息存储
    std::shared_ptr<Channel> channel_;
    std::weak_ptr<Timer> timer_; //weak_ptr，只观测Timer

    ConnectionState connectionState_;
    GeneralState generalState_;
    HeaderInternalState headerInternalState_;
    HTTPMethod method_;
    HTTPVersion version_;
    //std::string path_;

    const int expiredTime = 2000;
    const int keepAliveTime = 5 * 60 * 1000;
};