#ifndef DUDU_TCPSERVER_H
#define DUDU_TCPSERVER_H

#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

class EventLoop;

class TcpServer : private  noncopyable {
public:
    TcpServer(EventLoop* loop);
    ~TcpServer();

    static TcpServerPtr startServer(EventLoop* loop, const std::string ipport);

    void onConnCreate(const std::function<StreamSockPtr ()>& cb) { createcb_ = cb; }
    void onConnRead(const TcpCallBack& cb) { readcb_ = cb; }

    bool Bind(const SocketAddr& addr);
private:
    EventLoop* loop_;
    SocketAddr addr_;
    Channel* listen_channel_;
    std::function<StreamSockPtr ()> createcb_;
    TcpCallBack readcb_;

    int localSock_;
    short localPort_;

    void handleAccept();
};

#endif //DUDU_TCPSERVER_H
