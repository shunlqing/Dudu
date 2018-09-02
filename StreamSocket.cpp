#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <iostream>
#include <poll.h>

#include "StreamSocket.h"
#include "Channel.h"
#include "EventLoop.h"

using std::size_t;

StreamSocket::StreamSocket(EventLoop* loop)
    : loop_(loop)
{
}

StreamSocket::~StreamSocket()
{
//    std::cout << __FUNCTION__ << " peer ("
//        << peerAddr_.ToString()
//        << ")";
}

bool StreamSocket::Init(int fd, const SocketAddr& peer)
{
    if (fd < 0)
        return false;

    peerAddr_ = peer;
    localSock_ = fd;
    SetNonBlock(localSock_);
    state_ = State::Connected;

    //添加到相应的EventLoop
    channel_ = new Channel(loop_, fd, POLLIN | POLLOUT);

    StreamSockPtr ptr = shared_from_this();
    ptr->channel_->onReadable([=]{ ptr->handleRead(ptr); });
    ptr->channel_->onWritable([=]{ ptr->handleWrite(ptr); });

//    std::cout << __FUNCTION__ << " peer address ("
//        << peerAddr_.ToString()
//        << "), local socket is " << fd << std::endl;

    return  true;
}

void StreamSocket::cleanup(const StreamSockPtr &ptr) {
    if(readcb_ && recvBuf_.size()){
        readcb_(ptr);
    }

    state_ = State::Closed;

    readcb_ = writecb_ = nullptr;
    Channel* ch = channel_;
    channel_ = nullptr;
    delete ch;
}

bool StreamSocket::handleRead(const StreamSockPtr &ptr) {
    while(state_ == State::Connected) {
        //std::cout << "StreamSocket::handleRead" << std::endl;
        recvBuf_.makeRoom();
        std::size_t readlen = 0;
        if(channel_->getFd() >= 0) {
            readlen = ::read(channel_->getFd(), recvBuf_.end(), recvBuf_.space());
        }
        if(readlen == -1 && errno == EINTR) {
            continue;
        } else if(readlen == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if(readcb_ && recvBuf_.size()) {
                readcb_(ptr);
            }
            break;
        } else if(channel_->getFd() == -1 || readlen == 0 || readlen == -1) {
            cleanup(ptr);
            break;
        } else {
            recvBuf_.addSize(readlen);
        }
    }
}

bool StreamSocket::handleWrite(const StreamSockPtr &ptr) {
//    while(state_ == State::Connected) {
        ssize_t sended = isend(sendBuf_.begin(), sendBuf_.size());
        sendBuf_.consume(sended);
        if (sendBuf_.empty() && writecb_) {
            writecb_(ptr);
        }
        if (sendBuf_.empty() && channel_->writeEnabled()) { // writablecb_ may write something
            channel_->disableWrite();
        }
//    }
}

ssize_t StreamSocket::isend(const char* buf, size_t len) {
    size_t sended = 0;
    while (len > sended) {
        ssize_t wd = ::write(channel_->getFd(), buf + sended, len - sended);
        //printf("channel %lld fd %d write %ld bytes\n", (long long) channel_->getFd(), channel_->getFd(), wd);
        if (wd > 0) {
            sended += wd;
            continue;
        } else if (wd == -1 && errno == EINTR) {
            continue;
        } else if (wd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (!channel_->writeEnabled()) {
                channel_->enableWrite();
            }
            break;
        } else {
            printf("write error: channel %lld fd %d wd %ld %d %s", (long long) channel_->getFd(), channel_->getFd(), wd,
                   errno, strerror(errno));
            break;
        }
    }
    return sended;
}

void StreamSocket::send(const char *buf, size_t len) {
    if(channel_) {
        if(sendBuf_.empty()) {
            ssize_t sended = isend(buf, len);
            buf += sended;
            len -= sended;
        }
        if(len) {
            sendBuf_.append(buf, len);
        }
    } else {
        std::cout << "connection is closed, but still writing" << std::endl;
    }
}

void StreamSocket::send(Buffer &buf) {
    if(channel_) {
        if(channel_->writeEnabled()) {
            sendBuf_.absorb(buf);
        }
        if(buf.size()) {
            ssize_t sended = isend(buf.begin(), buf.size());
            buf.consume(sended);
        }
        if(buf.size()) {
            sendBuf_.absorb(buf);
            if(!channel_->writeEnabled()) {
                channel_->enableWrite();
            }
        }
    } else {
        std::cout << "connection is closed, but still writing" << std::endl;
    }
}

bool StreamSocket::OnError()
{
//    if (Socket::OnError())
//    {
//        ERR << __FUNCTION__ << " peer address ("
//            << peerAddr_.ToString()
//            << "), local socket is "
//            << localSock_;
//
//        if (onDisconnect_)
//            onDisconnect_();
//
//        return true;
//    }
//
//    return false;
}
