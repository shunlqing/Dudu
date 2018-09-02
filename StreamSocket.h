#ifndef DUDU_STREAMSOCKET_H
#define DUDU_STREAMSOCKET_H

#include "Socket.h"
#include "Buffer.h"
#include "EventLoop.h"

#include <sys/types.h>
#include <assert.h>
#include <sys/socket.h>

using PacketLength = int32_t;
class Channel;

// Abstraction for a TCP connection
class StreamSocket : public Socket, public std::enable_shared_from_this<StreamSocket>
{
public:
    StreamSocket(EventLoop* loop);
    ~StreamSocket() override;

    bool Init(int localfd, const SocketAddr& peer);

    Buffer& getInput() { return recvBuf_; }
    Buffer& getOutput() { return sendBuf_; }

    int GetFd() const { return localSock_; }
    Channel* GetChannel() const { return channel_; }

    SocketType GetSocketType() const override { return SocketType_Stream; }

public:
    // Send data
    void send(const char* buf, size_t len);
    void send(Buffer& buf);

    bool   OnReadable(const TcpCallBack& cb) { readcb_ = cb; };
    bool   OnWritable(const TcpCallBack& cb) { writecb_ = cb; };
    bool   OnError();

    bool   handleRead(const StreamSockPtr& ptr);
    bool   handleWrite(const StreamSockPtr& ptr);

    const SocketAddr& GetPeerAddr() const { return peerAddr_; }

    void cleanup(const StreamSockPtr& ptr);

private:

    ssize_t isend(const char* buf, size_t len);

    enum State
    {
        Invalid = 1,
        Connected,
        Closed
    };

    Buffer recvBuf_;
    Buffer sendBuf_;
    SocketAddr  peerAddr_;
    EventLoop* loop_;
    Channel* channel_;
    State state_;
    TcpCallBack readcb_, writecb_;
};

#endif //DUDU_STREAMSOCKET_H
