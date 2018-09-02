#include "Channel.h"
#include "EventLoop.h"
#include <unistd.h>
#include <sys/poll.h>

Channel::Channel(EventLoop *loop, int fd, int events)
    :loop_(loop), fd_(fd), events_(events)
{
    poller_ = loop->poller_;
    poller_->addChannel(this);
}

Channel::~Channel() {
    close();
}

void Channel::close() {
    if(fd_ >= 0) {
        poller_->removeChannel(this);
        ::close(fd_);
        fd_ = -1;
        handleRead();
    }
}

void Channel::enableRead() {
    events_ |= POLLIN;
    poller_->updateChannel(this);
}

void Channel::disableRead() {
    events_ &= ~POLLIN;
    poller_->updateChannel(this);
}

void Channel::enableWrite() {
    events_ |= POLLOUT;
    poller_->updateChannel(this);
}

void Channel::disableWrite() {
    events_ &= ~POLLOUT;
    poller_->updateChannel(this);
}

void Channel::enableReadWrite() {
    events_ |= POLLIN | POLLOUT;
    poller_->updateChannel(this);
}