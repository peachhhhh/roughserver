#include "Server.h"
#include "Socket.h"
#include "Channel.h"
#include "HTTPData.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

Server::Server(EventLoop *baseLoop, int numThread, int port)
    : start_(false),
      baseLoop_(baseLoop),
      numThread_(numThread),
      port_(port),
      eventLoopThreadPool_(new EventLoopThreadPool(baseLoop, numThread)),
      listenfd_(Socket::socketBindListen(port)),
      acceptChannel_(new Channel(baseLoop, listenfd_)),
      nullfd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
    //handle_for_sigpipe();
    if (Socket::setSocketNonBlocking(listenfd_) < 0)
    {
        //perror("set socket non block failed");
        //LOG
        abort();
    }
}

Server::~Server()
{
    ::close(nullfd_);
}

void Server::start()
{
    if (!start_)
    {
        start_ = true;
        eventLoopThreadPool_->start();
    }
    acceptChannel_->setEvents(EPOLLIN | EPOLLET);
    acceptChannel_->setReadCallback(std::bind(&Server::acceptNewConn, this));
    acceptChannel_->setModEpollfdEventCallback(std::bind(&Server::modEpollfdEvent, this));
    baseLoop_->addfd(acceptChannel_);
}

void Server::acceptNewConn()
{
    struct sockaddr_in clientAddr;
    memset(&clientAddr, 0, sizeof(struct sockaddr_in));
    socklen_t clientAddrLen = sizeof(clientAddr);
    int acceptfd = 0;
    while ((acceptfd = ::accept(listenfd_, (struct sockaddr *)&clientAddr, &clientAddrLen)) > 0) //由于是ET模式，需要读完为止，故使用while
    {
        EventLoop *eventLoop = eventLoopThreadPool_->getNextLoop();
        //LOG
        if (acceptfd >= 100000)
        {
            //FIXME 更改为保留一个空的描述符，防止socket描述符过多
            close(acceptfd);
        }
        if (Socket::setSocketNonBlocking(acceptfd) < 0)
        {
            //LOG
            return;
        }

        Socket::setSocketNodelay(acceptfd);

        std::shared_ptr<HTTPData> httpdata(new HTTPData(eventLoop, acceptfd));
        httpdata->getChannel()->setHTTPData(httpdata);
        eventLoop->queueInLoop(std::bind(&HTTPData::newEvent, httpdata)); //bind对参数拷贝，所以智能指针httpdata的引用加1，使得生命周期与EventLoop一致
    }
    acceptChannel_->setEvents(EPOLLIN | EPOLLET);
}
