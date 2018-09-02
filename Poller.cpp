#include "Poller.h"
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/poll.h>
#include <iostream>

EPoller* createPoller() { return new EPoller(); }

void EPoller::poll(int waitMs)
{
    lastActive_ = epoll_wait(epfd_, activeEvents_, kMaxEvents, waitMs);
    // TODO：记录日志
    while(--lastActive_ >= 0)
    {
        //std::cout << "handle event" << std::endl;
        int i = lastActive_;
        Channel* ch = (Channel*)activeEvents_[i].data.ptr;
        int events = activeEvents_[i].events;
        if(ch) {
            if(events & (POLLIN | POLLERR)) {
                ch->handleRead();
            } else if(events & (POLLOUT)) {
                ch->handleWrite();
            } else {
                //出错
            }
        }
    }
}

EPoller::EPoller()
{
    //初始化，创建一个epfd描述符
    epfd_ = epoll_create1(EPOLL_CLOEXEC);
    if(epfd_ < 0)
    {
        perror("epoll_create1");
    }
}

EPoller::~EPoller()
{
    for(auto ch : channels_)
    {
        ch->close();
    }
    ::close(epfd_);
}

void EPoller::addChannel(Channel * ch)
{
    struct epoll_event ev;
    ev.events = ch->getEvents();
    ev.data.ptr = ch;

    int r = epoll_ctl(epfd_, EPOLL_CTL_ADD, ch->getFd(), &ev);
    if(r < 0)
    {
        perror("epoll_ctl add failed");
    }

    channels_.insert(ch);
}

void EPoller::removeChannel(Channel * ch)
{
    channels_.erase(ch);
    for(int i = lastActive_; i >= 0; i--) {
        if(ch == activeEvents_[i].data.ptr) {
            activeEvents_[i].data.ptr = NULL;
            break;
        }
    }
    int r = epoll_ctl(epfd_, EPOLL_CTL_DEL, ch->getFd(), NULL);
    if(r < 0) {
        perror("epoll_ctl_del");
        std::cout << "channel_fd " << ch->getFd() << " " << epfd_ << std::endl;
    }
}

void EPoller::updateChannel(Channel * ch)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = ch->getEvents();
    ev.data.ptr = ch;

    int r = epoll_ctl(epfd_, EPOLL_CTL_MOD, ch->getFd(), &ev);
    if(r < 0) {
        perror("epoll_ctl_mod");
        std::cout << "channel_fd" << ch->getFd() << std::endl;
    }
}