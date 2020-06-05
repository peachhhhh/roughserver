#include "EventLoop.h"
#include "Server.h"

#include <getopt.h>
#include <string>

int main(int argc, char *argv[])
{
    int threadNum = 4;
    int port = 80;
    // parse args
    int opt;
    while ((opt = getopt(argc, argv, "t:p:")) != -1)
    {
        switch (opt)
        {
        case 't':
        {
            threadNum = atoi(optarg);
            break;
        }
        case 'p':
        {
            port = atoi(optarg);
            break;
        }
        default:
            break;
        }
    }
    EventLoop mainLoop;
    Server roughServer(&mainLoop, threadNum, port);
    roughServer.start();
    mainLoop.loop();
    return 0;
}
