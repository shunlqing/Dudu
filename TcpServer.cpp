#include <iostream>
#include <poll.h>
#include "TcpServer.h"
#include "StreamSocket.h"

TcpServer::TcpServer(EventLoop *loop)
    : loop_(loop),
      listen_channel_(NULL),
      createcb_( [=]{ return StreamSockPtr(new StreamSocket(loop)); })
{

}

TcpServer::~TcpServer() {
    delete listen_channel_;
}

bool TcpServer::Bind(const SocketAddr &addr) {
    if (addr.Empty())
        return  false;

    localPort_ = addr.GetPort();

    localSock_ = Socket::CreateTCPSocket();
    Socket::SetNonBlock(localSock_);
    Socket::SetNodelay(localSock_);
    Socket::SetReuseAddr(localSock_);
    Socket::SetRcvBuf(localSock_);
    Socket::SetSndBuf(localSock_);

    struct sockaddr_in serv = addr.GetAddr();

    int ret = ::bind(localSock_, (struct sockaddr*)&serv, sizeof serv);
    if (SOCKET_ERROR == ret)
    {
        Socket::CloseSocket(localSock_);
        std::cout << "Cannot bind port " << addr.GetPort();
        return false;
    }
    ret = ::listen(localSock_, 1024);
    if (SOCKET_ERROR == ret)
    {
        Socket::CloseSocket(localSock_);
        std::cout << "Cannot listen on port " << addr.GetPort();
        return false;
    }

    // 添加到相应的EventLoop
    listen_channel_ = new Channel(loop_, localSock_, POLLOUT | POLLIN);
    listen_channel_->onReadable([this] { handleAccept(); });

    std::cout << "Success: listen on ("
              << addr.GetIP()
              << ":"
              << addr.GetPort()
              << ")" << std::endl;

    return true;
}

void TcpServer::handleAccept() {
    int connfd;
    struct sockaddr_in raddr;
    socklen_t addrLength = sizeof raddr;
    while((connfd = ::accept(localSock_, (struct sockaddr *)&raddr, &addrLength)) >= 0)
    {
        struct sockaddr_in peer;
        socklen_t alen;
        int ret = getpeername(connfd, (struct sockaddr *) &peer, &alen);
        if (ret < 0) {
            std::cout << "getpeername error" << std::endl;
        }
        SocketAddr peerAddr(peer);

        EventLoop *loop = loop_->allocLoop();
        auto addStreamSocket = [=] {
            std::shared_ptr<StreamSocket> ptr = StreamSockPtr(new StreamSocket(loop));
            ptr->Init(connfd, peerAddr);

            if(readcb_) {
                ptr->OnReadable(readcb_);
            }
        };

        if(loop == loop_) {
            addStreamSocket();
        } else {
            loop->safeCall(std::move(addStreamSocket));
        }
    }

    //FIXME: error handle

}

TcpServerPtr TcpServer::startServer(EventLoop *loop, const std::string ipport) {
    TcpServerPtr p(new TcpServer(loop));
    bool ret = p->Bind(SocketAddr(ipport));
    if(!ret) {
        std::cout << "bind error" << std::endl;
        exit(1);
    }
    return ret == true ? p : NULL;
}

