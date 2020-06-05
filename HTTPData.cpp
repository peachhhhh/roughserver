#include "HTTPData.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "Timer.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

std::string findMimeType(const std::string &suffix)
{
    std::map<std::string, std::string>::const_iterator const_it = mimetype.find(suffix); //只能使用const_iterator迭代器访问
    if (const_it != mimetype.end())
    {
        return const_it->second;
    }
    else
    {
        return "text/html";
    }
}

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

void HTTPData::reset()
{
    //inBuffer_.clear();
    //path_.clear();
    //nowReadPos_ = 0;
    fileName_.clear();
    headers_.clear();
    generalState_ = PARSE_REQUEST_LINE;
    headerInternalState_ = START;
    // keepAlive_ = false;
    resetTimer();
}

void HTTPData::resetTimer()
{
    // cout << "seperateTimer" << endl;
    if (timer_.use_count())
    {
        std::shared_ptr<Timer> thisTimer(timer_.lock());
        thisTimer->resetHTTPSP();
        timer_.reset(); //weak_ptr重置
    }
}

void HTTPData::handleRead()
{
    unsigned int &events_ = channel_->getEvents();
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
            events_ |= EPOLLIN;
        }
    }
}

void HTTPData::handleWrite()
{
    if (!error_ && connectionState_ != DISCONNECTED)
    {
        unsigned int &events_ = channel_->getEvents();
        if (Socket::writefd(fd_, outBuffer_) < 0)
        {
            perror("writen");
            events_ = 0;
            error_ = true;
        }
        if (outBuffer_.size() > 0)
        {
            events_ |= EPOLLOUT;
        }
    }
}

void HTTPData::modEpollfdEventCallback()
{
    resetTimer();
    unsigned int &events_ = channel_->getEvents();
    if (!error_ && connectionState_ == CONNECTED)
    {
        if (events_ != 0)
        {
            int timeout = expiredTime;
            if (keepAlive_)
                timeout = keepAliveTime;
            if ((events_ & EPOLLIN) && (events_ & EPOLLOUT))
            {
                events_ = __uint32_t(0);
                events_ |= EPOLLOUT;
            }
            // events_ |= (EPOLLET | EPOLLONESHOT);
            events_ |= EPOLLET;
            eventLoop_->modfdEvent(channel_, timeout);
        }
        else if (keepAlive_)
        {
            events_ |= (EPOLLIN | EPOLLET);
            // events_ |= (EPOLLIN | EPOLLET | EPOLLONESHOT);
            int timeout = keepAliveTime;
            eventLoop_->modfdEvent(channel_, timeout);
        }
        else
        {
            // cout << "close normally" << endl;
            // loop_->shutdown(channel_);
            // loop_->runInLoop(bind(&HttpData::handleClose, shared_from_this()));
            events_ |= (EPOLLIN | EPOLLET);
            // events_ |= (EPOLLIN | EPOLLET | EPOLLONESHOT);
            int timeout = (keepAliveTime >> 1);
            eventLoop_->modfdEvent(channel_, timeout);
        }
    }
    else if (!error_ && connectionState_ == DISCONNECTING && (events_ & EPOLLOUT))
    {
        events_ = (EPOLLOUT | EPOLLET);
    }
    else
    {
        // cout << "close with errors" << endl;
        eventLoop_->runInLoop(std::bind(&HTTPData::closeConn, shared_from_this()));
    }
}

RequestLineState HTTPData::parseRequestLine() //解析请求行
{
    std::string &str = inBuffer_;
    // 读到完整的请求行再开始解析请求
    size_t pos = str.find('\r', 0);
    if (pos < 0)
    {
        return PARSE_REQUESTLINE_AGAIN;
    }
    // 去掉请求行所占的空间，节省空间
    std::string request_line = str.substr(0, pos);
    if (str.size() > pos + 1)
        str = str.substr(pos + 1);
    else
        str.clear();
    // Method
    int posGet = request_line.find("GET");
    int posPost = request_line.find("POST");
    int posHead = request_line.find("HEAD");

    if (posGet >= 0)
    {
        pos = posGet;
        method_ = GET;
    }
    else if (posPost >= 0)
    {
        pos = posPost;
        method_ = POST;
    }
    else if (posHead >= 0)
    {
        pos = posHead;
        method_ = HEAD;
    }
    else
    {
        return PARSE_REQUESTLINE_ERROR;
    }

    // filename
    pos = request_line.find("/", pos);
    if (pos < 0)
    {
        fileName_ = "index.html";
        version_ = HTTP11;
        return PARSE_REQUESTLINE_SUCCESS;
    }
    else
    {
        size_t _pos = request_line.find(' ', pos);
        if (_pos < 0)
            return PARSE_REQUESTLINE_ERROR;
        else
        {
            if (_pos - pos > 1)
            {
                fileName_ = request_line.substr(pos + 1, _pos - pos - 1);
                size_t __pos = fileName_.find('?');
                if (__pos >= 0)
                {
                    fileName_ = fileName_.substr(0, __pos);
                }
            }

            else
                fileName_ = "index.html";
        }
        pos = _pos;
    }
    // cout << "fileName_: " << fileName_ << endl;
    // HTTP 版本号
    pos = request_line.find("/", pos);
    if (pos < 0)
        return PARSE_REQUESTLINE_ERROR;
    else
    {
        if (request_line.size() - pos <= 3)
            return PARSE_REQUESTLINE_ERROR;
        else
        {
            std::string ver = request_line.substr(pos + 1, 3);
            if (ver == "1.0")
                version_ = HTTP10;
            else if (ver == "1.1")
                version_ = HTTP11;
            else
                return PARSE_REQUESTLINE_ERROR;
        }
    }
    return PARSE_REQUESTLINE_SUCCESS;
}

HeaderState HTTPData::parseHeaders()
{
    std::string &str = inBuffer_;
    int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
    int now_read_line_begin = 0;
    bool notFinish = true;
    size_t i = 0;
    for (; i < str.size() && notFinish; ++i)
    {
        switch (headerInternalState_)
        {
        case START:
        {
            if (str[i] == '\n' || str[i] == '\r')
                break;
            headerInternalState_ = KEY;
            key_start = i;
            now_read_line_begin = i;
            break;
        }
        case KEY:
        {
            if (str[i] == ':')
            {
                key_end = i;
                if (key_end - key_start <= 0)
                    return PARSE_HEADER_ERROR;
                headerInternalState_ = COLON;
            }
            else if (str[i] == '\n' || str[i] == '\r')
                return PARSE_HEADER_ERROR;
            break;
        }
        case COLON:
        {
            if (str[i] == ' ')
            {
                headerInternalState_ = SPACES_AFTER_COLON;
            }
            else
                return PARSE_HEADER_ERROR;
            break;
        }
        case SPACES_AFTER_COLON:
        {
            headerInternalState_ = VALUE;
            value_start = i;
            break;
        }
        case VALUE:
        {
            if (str[i] == '\r')
            {
                headerInternalState_ = CR;
                value_end = i;
                if (value_end - value_start <= 0)
                    return PARSE_HEADER_ERROR;
            }
            else if (i - value_start > 255)
                return PARSE_HEADER_ERROR;
            break;
        }
        case CR:
        {
            if (str[i] == '\n')
            {
                headerInternalState_ = LF;
                std::string key(str.begin() + key_start, str.begin() + key_end);
                std::string value(str.begin() + value_start, str.begin() + value_end);
                headers_[key] = value;
                now_read_line_begin = i;
            }
            else
                return PARSE_HEADER_ERROR;
            break;
        }
        case LF:
        {
            if (str[i] == '\r')
            {
                headerInternalState_ = END_CR;
            }
            else
            {
                key_start = i;
                headerInternalState_ = KEY;
            }
            break;
        }
        case END_CR:
        {
            if (str[i] == '\n')
            {
                headerInternalState_ = END_LF;
            }
            else
                return PARSE_HEADER_ERROR;
            break;
        }
        case END_LF:
        {
            notFinish = false;
            key_start = i;
            now_read_line_begin = i;
            break;
        }
        }
    }
    if (headerInternalState_ == END_LF)
    {
        str = str.substr(i);
        return PARSE_HEADER_SUCCESS;
    }
    str = str.substr(now_read_line_begin);
    return PARSE_HEADER_AGAIN;
}

AnalysisState HTTPData::analysisRequest()
{
    if (method_ == POST)
    {
        // ------------------------------------------------------
        // My CV stitching handler which requires OpenCV library
        // ------------------------------------------------------
        // string header;
        // header += string("HTTP/1.1 200 OK\r\n");
        // if(headers_.find("Connection") != headers_.end() &&
        // headers_["Connection"] == "Keep-Alive")
        // {
        //     keepAlive_ = true;
        //     header += string("Connection: Keep-Alive\r\n") + "Keep-Alive:
        //     timeout=" + to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
        // }
        // int length = stoi(headers_["Content-length"]);
        // vector<char> data(inBuffer_.begin(), inBuffer_.begin() + length);
        // Mat src = imdecode(data, CV_LOAD_IMAGE_ANYDEPTH|CV_LOAD_IMAGE_ANYCOLOR);
        // //imwrite("receive.bmp", src);
        // Mat res = stitch(src);
        // vector<uchar> data_encode;
        // imencode(".png", res, data_encode);
        // header += string("Content-length: ") + to_string(data_encode.size()) +
        // "\r\n\r\n";
        // outBuffer_ += header + string(data_encode.begin(), data_encode.end());
        // inBuffer_ = inBuffer_.substr(length);
        // return ANALYSIS_SUCCESS;
    }
    else if (method_ == GET || method_ == HEAD)
    {
        std::string header;
        header += "HTTP/1.1 200 OK\r\n";
        if (headers_.find("Connection") != headers_.end() &&
            (headers_["Connection"] == "Keep-Alive" ||
             headers_["Connection"] == "keep-alive"))
        {
            keepAlive_ = true;
            header += std::string("Connection: Keep-Alive\r\n") + "Keep-Alive: timeout=" +
                      std::to_string(keepAliveTime) + "\r\n";
        }
        int dot_pos = fileName_.find('.');
        std::string filetype;
        if (dot_pos < 0)
            filetype = findMimeType("default");
        else
            filetype = findMimeType(fileName_.substr(dot_pos));

        // echo test
        if (fileName_ == "hello")
        {
            outBuffer_ =
                "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\nHello World";
            return ANALYSIS_SUCCESS;
        }

        struct stat statbuf; //文件属性
        if (::stat(fileName_.c_str(), &statbuf) < 0)
        {
            header.clear();
            handleError(fd_, 404, "Not Found!");
            return ANALYSIS_ERROR;
        }
        header += "Content-Type: " + filetype + "\r\n";
        header += "Content-Length: " + std::to_string(statbuf.st_size) + "\r\n";
        header += "Server: LinYa's Web Server\r\n";
        // 头部结束
        header += "\r\n";
        outBuffer_ += header;

        if (method_ == HEAD)
            return ANALYSIS_SUCCESS;

        int src_fd = ::open(fileName_.c_str(), O_RDONLY, 0);
        if (src_fd < 0)
        {
            outBuffer_.clear();
            handleError(fd_, 404, "Not Found!");
            return ANALYSIS_ERROR;
        }
        void *mmapRet = ::mmap(NULL, statbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0); //将文件映射到内存的存储区，返回起始地址
        ::close(src_fd);
        if (mmapRet == (void *)-1)
        {
            ::munmap(mmapRet, statbuf.st_size); //解除映射
            outBuffer_.clear();
            handleError(fd_, 404, "Not Found!");
            return ANALYSIS_ERROR;
        }
        char *src_addr = static_cast<char *>(mmapRet);
        outBuffer_ += std::string(src_addr, src_addr + statbuf.st_size);
        ;
        ::munmap(mmapRet, statbuf.st_size);
        return ANALYSIS_SUCCESS;
    }
    return ANALYSIS_ERROR;
}

void HTTPData::handleError(int fd, int err_num, std::string short_msg)
{
    short_msg = " " + short_msg;
    std::string body_buff, header_buff;

    header_buff += "HTTP/1.1 " + std::to_string(err_num) + short_msg + "\r\n";
    header_buff += "Content-Type: text/html\r\n";
    header_buff += "Connection: Close\r\n";
    header_buff += "Content-Length: " + std::to_string(body_buff.size()) + "\r\n";
    header_buff += "Server: roughserver\r\n";
    header_buff += "\r\n";

    body_buff += "<html><title>哎~出错了</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += std::to_string(err_num) + short_msg;
    body_buff += "<hr><em> roughserver</em>\n</body></html>";

    Socket::writefd(fd, header_buff);
    Socket::writefd(fd, body_buff);
}