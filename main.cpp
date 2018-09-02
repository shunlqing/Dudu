#include <iostream>
#include <thread>
#include "EventLoop.h"
#include "TcpServer.h"
#include "StreamSocket.h"
#include "ThreadPool.h"

int main(int argc, char* argv[]) {
    int threadCnt = 1;
    if(argc != 2) {
        threadCnt = 1;
    } else {
        threadCnt = atoi(argv[1]);
    }

    EventLoop *loop;
    if(threadCnt == 1)
        loop = new EventLoop();
    else
        loop = new MultiEventLoop(threadCnt);
    // TODO: 信号处理

    TcpServerPtr svr = TcpServer::startServer(loop, "127.0.0.1:8088");
    if(NULL == svr) {
        std::cout << "start tcp server failed" << std::endl;
        exit(1);
    }
    svr->onConnRead([] (const StreamSockPtr& ptr) {
        //std::cout << "recv data" << std::endl;
        ptr->send(ptr->getInput());
    });

    loop->loop();
}