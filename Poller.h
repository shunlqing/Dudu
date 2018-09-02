#ifndef DUDU_POLLER_H
#define DUDU_POLLER_H

#include "Util.h"
#include "Channel.h"
#include <sys/epoll.h>
#include <set>

const int kMaxEvents = 2000;

class EPoller : private noncopyable {
public:
    EPoller();
    ~EPoller();

    void addChannel(Channel* ch);
    void removeChannel(Channel* ch);
    void updateChannel(Channel* ch);
    void poll(int timeout);

public:
    int epfd_;
    std::set<Channel*> channels_;
    struct epoll_event activeEvents_[kMaxEvents];
    int lastActive_;
};

EPoller* createPoller();

#endif //DUDU_POLLER_H
