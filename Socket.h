#pragma once

namespace Socket
{
    int socketBindListen(int port);
    int setSocketNonBlocking(int fd);
    void setSocketNodelay(int fd);

    ssize_t readfd(int fd, void *buf, size_t count); //wakeupfd
    ssize_t readfd(int fd, std::string &inBuffer, bool &isZero); //ET模式，读到不能再读，即遇到EAGAIN
    ssize_t writefd(int fd, void *buf, size_t count);
} // namespace Socket