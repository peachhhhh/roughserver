#include "HTTPData.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"

HTTPData::HTTPData(EventLoop *eventLoop, int fd)
    : eventLoop_(eventLoop),
      fd_(fd),
      error_(false),
      keepAlive_(false),
      channel_(new Channel(eventLoop, fd)),
      connectionState_(CONNECTED),
      generalState_(PARSE_REQUEST_LINE),
      headerInternalState_(START),
      method_(GET),
      version_(HTTP11)
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
    connectionState_ = DISCONNECTED;
    eventLoop_->delfd(channel_);
}

void HTTPData::handleRead()
{
    /////////int &events_ = channel_->getEvents();
    do
    {
        bool isZero = false;
        int readSum = Socket::readfd(fd_, inBuffer_, isZero);
        //LOG << "Request: " << inBuffer_;
        if (connectionState_ == DISCONNECTING)
        {
            inBuffer_.clear();
            break;
        }
        // cout << inBuffer_ << endl;
        if (readSum < 0)
        {
            perror("1");
            error_ = true;
            handleError(fd_, 400, "Bad Request");
            break;
        }
        // else if (read_num == 0)
        // {
        //     error_ = true;
        //     break;
        // }
        else if (isZero)
        {
            // 有请求出现但是读不到数据，可能是Request
            // Aborted，或者来自网络的数据没有达到等原因
            // 最可能是对端已经关闭了，统一按照对端已经关闭处理
            // error_ = true;
            connectionState_ = DISCONNECTING;
            if (readSum == 0)
            {
                // error_ = true;
                break;
            }
            // cout << "readnum == 0" << endl;
        }

        if (generalState_ == PARSE_REQUEST_LINE)
        {
            RequestLineState flag = this->parseRequestLine();
            if (flag == PARSE_REQUESTLINE_AGAIN)
                break;
            else if (flag == PARSE_REQUESTLINE_ERROR)
            {
                perror("2");
                //LOG << "FD = " << fd_ << "," << inBuffer_ << "******";
                inBuffer_.clear();
                error_ = true;
                handleError(fd_, 400, "Bad Request");
                break;
            }
            else
                generalState_ = PARSE_HEADERS;
        }
        if (generalState_ == PARSE_HEADERS)
        {
            HeaderState flag = this->parseHeaders();
            if (flag == PARSE_HEADER_AGAIN)
                break;
            else if (flag == PARSE_HEADER_ERROR)
            {
                perror("3");
                error_ = true;
                handleError(fd_, 400, "Bad Request");
                break;
            }
            if (method_ == POST)
            {
                generalState_ = RECV_BODY; //post，则接受请求体
            }
            else
            {
                generalState_ = ANALYSIS;
            }
        }
        if (generalState_ == RECV_BODY)
        {
            int content_length = -1;
            if (headers_.find("Content-length") != headers_.end())
            {
                content_length = std::stoi(headers_["Content-length"]);
            }
            else
            {
                // cout << "(state_ == STATE_RECV_BODY)" << endl;
                error_ = true;
                handleError(fd_, 400, "Bad Request: Lack of argument (Content-length)");
                break;
            }
            if (static_cast<int>(inBuffer_.size()) < content_length)
            {
                break;
            }
            generalState_ = ANALYSIS;
        }
        if (generalState_ == ANALYSIS)
        {
            AnalysisState flag = this->analysisRequest();
            if (flag == ANALYSIS_SUCCESS)
            {
                generalState_ = FINISH;
                break;
            }
            else
            {
                // cout << "state_ == STATE_ANALYSIS" << endl;
                error_ = true;
                break;
            }
        }
    } while (false);
    // cout << "state_=" << state_ << endl;
    if (!error_)
    {
        if (outBuffer_.size() > 0)
        {
            handleWrite();
            // events_ |= EPOLLOUT;
        }
        // error_ may change
        if (!error_ && generalState_ == FINISH)
        {
            this->reset();
            if (inBuffer_.size() > 0)
            {
                if (connectionState_ != DISCONNECTING)
                    handleRead();
            }

            // if ((keepAlive_ || inBuffer_.size() > 0) && connectionState_ ==
            // H_CONNECTED)
            // {
            //     this->reset();
            //     events_ |= EPOLLIN;
            // }
        }
        else if (!error_ && connectionState_ != DISCONNECTED)
        {
            //////events_ |= EPOLLIN;
        }
    }
}
