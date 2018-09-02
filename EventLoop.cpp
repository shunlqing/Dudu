#include "EventLoop.h"
#include "Timer.h"
#include "Poller.h"
#include <vector>
#include <thread>

EventLoop::EventLoop()
    : poller_(createPoller()), exit_(false), nextTimeout_(1 << 30)// FIXME
{
    int ret = pipe(wakeupFds_);
    if(ret < 0) {
        std::cout << "pipe error" << std::endl;
    }

    Channel *ch = new Channel(this, wakeupFds_[0], POLLIN);
    ch->onReadable([=] {
        char buf[1024];
        int r = ch->getFd() >= 0 ? ::read(ch->getFd(), buf, sizeof(buf)) : 0;
        if(r > 0) {
            Task task;
            while(tasks_.pop_wait(&task, 0)) { // 0为非阻塞
                task();
            }
        } else if(r == 0) {
            delete ch;
        } else if(errno == EINTR) {

        } else {
            std::cout << "wakeup channel read error" << std::endl;
        }
    });
}

EventLoop::~EventLoop()
{

}

void EventLoop::loop()
{
    std::cout << "looping" << std::endl;
    while(!exit_) {
        int waitMs = std::min(nextTimeout_, 1000);
        poller_->poll(waitMs);

        handleTimeouts();
    }
}

void EventLoop::handleTimeouts()
{
    Time checktime;
    checktime.Now();
    timers_.UpdateTimers(checktime);
}

void EventLoop::runAt(int64_t milli, Task &&task, int64_t internal, int64_t count)
{
    auto timer = timers_.CreateTimer();
    timer->Init(internal, count);
    timer->SetCallback(task);
    timers_.AddTimer(timer);
}

EventLoop* MultiEventLoop::allocLoop()
{
    int c = id_++;
    int idx = c%(loops_.size()+1);
    return (idx == loops_.size()) ? static_cast<EventLoop*>(this) : &loops_[idx];
}

void MultiEventLoop::loop() {
    int sz = loops_.size();
    std::vector<std::thread> ths(sz);
    for(int i = 0; i < sz; i++) {
        std::thread t([this, i]{ loops_[i].loop(); });
        ths[i].swap(t);
    }

    EventLoop::loop();
    for(int i = 0; i < sz; i++) {
        ths[i].join();
    }
}

void MultiEventLoop::Stop()
{
    for(auto &it : loops_) {
        it.Stop();
    }
    EventLoop::Stop();
}

