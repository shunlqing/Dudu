#ifndef DUDU_CHANNEL_H
#define DUDU_CHANNEL_H

#include "Util.h"
#include <functional>
#include <poll.h>
#include <iostream>

class EPoller;
class EventLoop;

class Channel : public noncopyable {
public:
    Channel(EventLoop* loop, int fd, int events);
    ~Channel();

    // 获取成员函数
    EventLoop* getLoop() { return loop_; }
    int getFd() { return fd_; }
//    int64_t getId() { return id_; }
    short getEvents() { return events_; }

    void close();

    //注册回调函数
    void onReadable(const Task& readcb) { readcb_ = readcb; }
    void onWritable(const Task& writecb) { writecb_ = writecb; }
    void onReadable(Task&& readcb) { readcb_ = std::move(readcb); }
    void onWritable(Task&& writecb) { writecb_ = std::move(writecb); }

    // 启动监听
    void enableRead();
    void enableWrite();
    void disableRead();
    void disableWrite();
    void enableReadWrite();

    bool writeEnabled() { return events_ & POLLOUT; }
    bool readEnabled() { return events_ & POLLIN; }

    // 处理读写事件
    void handleRead() {
        if(readcb_)
            readcb_();
        else
            std::cout << "Read Callback never be set" << std::endl;
    }
    void handleWrite() {
        if(writecb_)
            writecb_();
        else
            std::cout << "Write Callback never be set" << std::endl;
    }

private:
    EventLoop* loop_;
    EPoller* poller_;
    int fd_;
    short events_;
    std::function<void ()> readcb_, writecb_, errorcb_;
};

#endif //DUDU_CHANNEL_H
