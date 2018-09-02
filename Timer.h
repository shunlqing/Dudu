#ifndef DUDU_TIMER_H
#define DUDU_TIMER_H

#include <vector>
#include <set>
#include <ctime>
#include <sys/time.h>
#include <stdint.h>
#include <memory>
#include <mutex>
#include <atomic>

uint64_t Now();

class Time
{
public:
    Time();
    Time(const Time& other);
    Time(int hour, int min, int sec);

    void        Now();
    uint64_t    MilliSeconds() const { return ms_; }
    void        AddDelay(uint64_t delay);

    int GetYear()   const { _UpdateTm(); return tm_.tm_year + 1900;  }
    int GetMonth()  const { _UpdateTm(); return tm_.tm_mon + 1;  }
    int GetDay()    const { _UpdateTm(); return tm_.tm_mday; }
    int GetHour()   const { _UpdateTm(); return tm_.tm_hour; }
    int GetMinute() const { _UpdateTm(); return tm_.tm_min;  }
    int GetSecond() const { _UpdateTm(); return tm_.tm_sec;  }

    Time&    operator= (const Time& );
    operator uint64_t() const { return ms_; }

private:
    uint64_t    ms_;      // milliseconds from 1970

    mutable tm  tm_;
    mutable bool valid_; // 表示tm_能否表示ms_

    void    _UpdateTm()  const;
};


class Timer
{
    friend class TimerQueue;
public:
    void  Init(uint32_t interval, int32_t count = -1);
    bool  OnTimer();
    void  SetRemainCnt(int32_t remain) {  count_ = remain; }
    bool  IsWorking() const {  return  prev_ != nullptr; }

    template <typename F, typename... Args>
    void SetCallback(F&& f, Args&&... args)
    {
        auto temp = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        func_ = [temp]() { (void)temp(); };
    }

private:
    Timer();

    std::function<void ()> func_;
    Timer* next_;
    Timer* prev_;
    Time     triggerTime_; //绝对触发时间
    uint32_t interval_; //触发间隔时间
    int32_t  count_; // -1 表示重复定时器（默认），>0表示重复次数
};

class TimerQueue
{
public:
    TimerQueue();
    ~TimerQueue();

    TimerQueue(const TimerQueue& ) = delete;
    void operator= (const TimerQueue& ) = delete;

    bool    UpdateTimers(const Time& now);
    void    ScheduleAt(Timer* pTimer, const Time& triggerTime);
    void    AddTimer(Timer* timer);
    void    KillTimer(Timer* pTimer);

    Timer*  CreateTimer();

private:

    bool _Cacsade(Timer* pList[], int index);
    int _Index(int level);

    static const int LIST1_BITS = 8;
    static const int LIST_BITS  = 6;
    static const int LIST1_SIZE = 1 << LIST1_BITS;
    static const int LIST_SIZE  = 1 << LIST_BITS;

    Time  m_lastCheckTime;

    Timer* m_list1[LIST1_SIZE];
    Timer* m_list2[LIST_SIZE];
    Timer* m_list3[LIST_SIZE];
    Timer* m_list4[LIST_SIZE];
    Timer* m_list5[LIST_SIZE];

    int timerCnt_;

    // timer pool
    std::set<Timer* > freepool_;
};

inline int TimerQueue::_Index(int level)
{
    uint64_t current = m_lastCheckTime;
    current >>= (LIST1_BITS + level * LIST_BITS);
    return current & (LIST_SIZE - 1);
}

#endif //DUDU_TIMER_H
