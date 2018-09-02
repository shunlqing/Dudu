#include "TCPClient.h"

TCPClient::TCPClient(EventLoop *loop, std::string ipport)
    : loop_(loop), peerAddr_(ipport), conPtr_(nullptr)
{
}

void TCPClient::Connect()
{
    // 建立连接, 并添加到相应的loop
    SocketAddr addr(peerAddr_);

    int sockfd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        std::cout << "socket failed" << std::endl;
        exit(1);
    }

    struct sockaddr_in servaddr = addr.GetAddr();
    int ret;
    ret = ::connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if(ret < 0) {
        std::cout << "connect failed" << std::endl;
        exit(1);
    }

    conPtr_ = StreamSockPtr(new StreamSocket(loop_));
    conPtr_->Init(sockfd, addr);

    if(msgcb_) {
        conPtr_->OnReadable(msgcb_);
    }

    if(conncb_) {
        conncb_();
    }
}

void TCPClient::DisConnect()
{
    conPtr_->cleanup(conPtr_);
    if(disconncb_) {
        disconncb_();
    }
}

