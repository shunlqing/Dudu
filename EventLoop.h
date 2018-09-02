#ifndef DUDU_EVENTLOOP_H
#define DUDU_EVENTLOOP_H

#include "Util.h"
#include "Poller.h"
#include "SafeQueue.h"
#include "Timer.h"

#include <atomic>
#include <vector>
#include <memory>
#include <iostream>
#include <unistd.h>

class StreamSocket;
class TcpServer;

typedef std::shared_ptr<StreamSocket> StreamSockPtr;
typedef std::function<void (const StreamSockPtr)> TcpCallBack;
typedef std::shared_ptr<TcpServer> TcpServerPtr;

struct EventLoopBase {
    virtual EventLoop* allocLoop() = 0;
    virtual void loop() = 0;
    virtual ~EventLoopBase() = default;
};

class EventLoop : public EventLoopBase {
public:
    EventLoop();
    ~EventLoop();

    virtual void loop();
    virtual void Stop() { exit_ = true; }

    //定时任务相关
    void runAt(int64_t milli, Task&& task, int64_t internal = 0, int64_t count = -1);
    void runAt(int64_t milli, const Task& task, int64_t internal = 0, int64_t count = -1) { runAt(milli, Task(task), internal, count); }
    void runAfter(int64_t milli, Task&& task, int64_t internal = 0, int64_t count = -1) { runAt(util::timeMilli() + milli, std::move(task), internal, count); }
    void runAfter(int64_t milli, const Task& task, int64_t internal = 0, int64_t count = -1) { runAt(util::timeMilli() + milli, Task(task), internal, count); }
    void handleTimeouts();

    //安全队列相关的
    void safeCall(Task&& task) { tasks_.push(std::move(task)); wakeup();}
    void safeCall(const Task& task) { safeCall(Task(task)); }
    void wakeup() {
        int ret = write(wakeupFds_[1], "", 1);
        if(ret < 0) {
            std::cout << "write error in wakup" << std::endl;
        }
    }

    void removeChannel(Channel* channel) { poller_->removeChannel(channel); }
    void updateChannel(Channel* channel) { poller_->updateChannel(channel); }

    virtual EventLoop* allocLoop() { return this; }

public:
    EPoller* poller_;
    int wakeupFds_[2];
    int nextTimeout_;
    SafeQueue<Task> tasks_;
    TimerQueue timers_;
    std::atomic<bool> exit_;
};

class MultiEventLoop : public EventLoop {
public:
    MultiEventLoop(int sz) : id_(0), loops_(sz-1) {}

    virtual EventLoop* allocLoop();
    virtual void loop() override;

    void Stop();

private:
    std::atomic<int> id_;
    std::vector<EventLoop> loops_;
};

#endif //DUDU_EVENTLOOP_H
