#pragma once

#include "Socket.h"

#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <memory.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

const int MAX_BUFF = 4096;

int Socket::socketBindListen(int port)
{
    int socketfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (socketfd < 0)
    {
        //LOG_SYSFATAL
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);
    if (::bind(socketfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        //LOG_SYSFATAL
    }
    if (::listen(socketfd, SOMAXCONN) < 0)
    {
        //LOG_SYSFATAL
    }
    return socketfd;
}

int Socket::setSocketNonBlocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0); //获取描述符原来的flag
    if (flag == -1)
    {
        return -1;
    }
    flag |= O_NONBLOCK;                 //设置为非阻塞socketfd
    if (fcntl(fd, F_SETFL, flag) == -1) //设置flag
    {
        return -1;
    }
    return 0;
}

void Socket::setSocketNodelay(int fd)
{
    int enable = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&enable, sizeof(enable)); //禁用Nagle算法，允许小包发送，降低延时
}

ssize_t Socket::readfd(int fd, void *buf, size_t count) //wakeupfd
{
    return ::read(fd, buf, count);
}

ssize_t Socket::readfd(int fd, std::string &inBuffer, bool &isZero) //ET模式，读到不能再读，即遇到EAGAIN
{
    ssize_t hasRead = 0;
    ssize_t readSum = 0;
    while (true)
    {
        char buff[MAX_BUFF];
        if ((hasRead = ::read(fd, buff, MAX_BUFF)) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            else if (errno == EAGAIN)
            {
                return readSum;
            }
            else
            {
                perror("read error");
                return -1;
            }
        }
        else if (hasRead == 0)
        {
            // printf("redsum = %d\n", readSum);
            isZero = true;
            break;
        }
        // printf("before inBuffer.size() = %d\n", inBuffer.size());
        // printf("nread = %d\n", nread);
        readSum += hasRead;
        // buff += nread;
        inBuffer += std::string(buff, buff + hasRead);
        // printf("after inBuffer.size() = %d\n", inBuffer.size());
    }
    return readSum;
}

ssize_t Socket::writefd(int fd, const void *buf, size_t count)
{
    return ::write(fd, buf, count);
}