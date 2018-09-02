#ifndef DUDU_TCPCLIENT_H
#define DUDU_TCPCLIENT_H

#include "EventLoop.h"
#include "StreamSocket.h"

class TCPClient {
public:
    TCPClient(EventLoop* loop, std::string ipport);
    ~TCPClient(){}

    void Connect();
    void DisConnect();

    void OnConnect(const std::function<void()>& cb) { conncb_ = cb; }
    void OnConnect(std::function<void ()>&& cb) { conncb_ = std::move(cb); }
    void OnDisConnect(const std::function<void()>& cb) { disconncb_ = cb; }
    void OnDisConnect(std::function<void ()>&& cb) { disconncb_ = std::move(cb); }
    void OnMessage(const TcpCallBack& cb) { msgcb_ = cb; }
    void OnMessage(TcpCallBack&& cb) { msgcb_ = std::move(cb); }

    StreamSocket& GetConn() { return *conPtr_; }
    EventLoop* GetLoop() const { return loop_; }

private:
    EventLoop* loop_;
    StreamSockPtr conPtr_;
    std::string peerAddr_;
    std::function<void ()> conncb_, disconncb_;
    TcpCallBack msgcb_;
};

#endif //DUDU_TCPCLIENT_H
