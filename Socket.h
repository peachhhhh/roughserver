#pragma once

namespace Socket
{
    int socketBindListen(int port);
    int setSocketNonBlocking(int fd);
    void setSocketNodelay(int fd);

    ssize_t readfd(int fd, void *buf, size_t count);
    ssize_t writefd(int fd, void *buf, size_t count);
}