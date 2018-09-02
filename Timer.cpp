#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cstring>
#include "Timer.h"

uint64_t Now()
{
    struct timeval now;
    ::gettimeofday(&now, 0);
    return  uint64_t(now.tv_sec * 1000UL + now.tv_usec / 1000UL);
}

static bool IsLeapYear(int year)
{
    return  (year % 400 == 0 ||
             (year % 4 == 0 && year % 100 != 0));
}

static int DaysOfMonth(int year, int month)
{
    const int monthDay[13] = {-1, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (2 == month && IsLeapYear(year))
        return 29;

    return  monthDay[month];
}



Time::Time() : ms_(0), valid_(false)
{
    tm_.tm_year = 0;
    this->Now();
}

Time::Time(const Time& other) : ms_(other.ms_)
{
    tm_.tm_year = 0;
    valid_ = false;
}

// 给定时，分，秒
// 设定一个时间，需要做的是，要判断是今天还是明天?
// 如果今天这个时间点已经过了，那么就是明天了
Time::Time(int hour, int min, int sec)
{
    this->Now();

    //如果hour小于当前的hour，则换天了，如果当前天数是本月最后一天，则换月；如果本月是12月，则换年
    int day   = GetDay();
    int mon   = GetMonth();
    int year  = GetYear();

    bool  tomorrow = false;
    if (hour < GetHour() ||
        (hour == GetHour() && min < GetMinute()) ||
        (hour == GetHour() && min == GetMinute() && sec < GetSecond()))
        tomorrow = true;

    if (tomorrow)
    {
        if (DaysOfMonth(year, mon) == day)
        {
            day = 1;

            if (12 == mon)
            {
                mon = 1;
                ++ year;
            }
            else
                ++ mon;
        }
        else
        {
            ++ day;
        }
    }

    // 构造tm
    struct tm  stm;
    stm.tm_sec = sec;
    stm.tm_min = min;
    stm.tm_hour= hour;
    stm.tm_mday= day;
    stm.tm_mon = mon - 1;
    stm.tm_year = year - 1900;
    stm.tm_yday = 0;
    stm.tm_isdst = 0;

    time_t tt = mktime(&stm); // 时间转化成秒数
    ms_ = tt * 1000UL;
    valid_ = false;
}

void Time::_UpdateTm()  const
{
    if (valid_)
        return;

    valid_ = true;
    const time_t now(ms_ / 1000UL);
    ::localtime_r(&now, &tm_);
}

void Time::Now()
{
    ms_ = ::Now();
    valid_ = false;
}

// from 2015 to 2025
static const char* YEAR[] = { "2015", "2016", "2017", "2018",
                              "2019", "2020", "2021", "2022", "2023", "2024", "2025",
                              nullptr,
};

void Time::AddDelay(uint64_t delay)
{
    ms_   += delay;
    valid_ = false;
}

Time& Time::operator= (const Time & other)
{
    if (this != &other)
    {
        ms_    = other.ms_;
        valid_ = false;
    }
    return *this;
}

Timer::Timer(): next_(nullptr), prev_(nullptr)
{
}

void  Timer::Init(uint32_t interval, int32_t count)
{
    interval_ = interval;
    count_ = count; //count == -1 表示循环定时
    triggerTime_.Now();
    triggerTime_.AddDelay(interval);
}

bool Timer::OnTimer()
{
    if (!func_)
        return false;

    if (count_ < 0 || -- count_ >= 0) // count为0时，进入判断为false
    {
        triggerTime_.AddDelay(interval_);
        func_();

        return true;
    }

    return false;
}

TimerQueue::TimerQueue() : timerCnt_(0)
{
    for (int i = 0; i < LIST1_SIZE; ++ i)
    {
        m_list1[i] = new Timer();
    }

    for (int i = 0; i < LIST_SIZE; ++ i)
    {
        m_list2[i] = new Timer();
        m_list3[i] = new Timer();
        m_list4[i] = new Timer();
        m_list5[i] = new Timer();
    }
}

TimerQueue::~TimerQueue()
{
    Timer* pTimer = nullptr;
    for (int i = 0; i < LIST1_SIZE; ++ i)
    {
        while ((pTimer = m_list1[i]->next_) )
        {
            KillTimer(pTimer);
            delete pTimer;
        }

        delete m_list1[i];
    }

    for (int i = 0; i < LIST_SIZE; ++ i)
    {
        while ((pTimer = m_list2[i]->next_) )
        {
            KillTimer(pTimer);
            delete pTimer;
        }
        delete m_list2[i];

        while ((pTimer = m_list3[i]->next_) )
        {
            KillTimer(pTimer);
            delete pTimer;
        }
        delete m_list3[i];

        while ((pTimer = m_list4[i]->next_) )
        {
            KillTimer(pTimer);
            delete pTimer;
        }
        delete m_list4[i];

        while ((pTimer = m_list5[i]->next_) )
        {
            KillTimer(pTimer);
            delete pTimer;
        }
        delete m_list5[i];
    }

    for (auto t : freepool_)
        delete t;
}

Timer* TimerQueue::CreateTimer()
{
    Timer* timer = nullptr;
    if (freepool_.empty())
    {
        timer = new Timer();
    }
    else
    {
        timer = *(freepool_.begin());
        freepool_.erase(freepool_.begin());
    }

    return timer;
}

bool TimerQueue::UpdateTimers(const Time& now)
{
    const bool hasUpdated(m_lastCheckTime <= now);

    while (m_lastCheckTime <= now) //处理每一ms的定时器队列
    {
        int index = m_lastCheckTime & (LIST1_SIZE - 1);
        if (index == 0 && //index==0说明本次处理将跨越256ms，需要将
            !_Cacsade(m_list2, _Index(0)) && //将16s盘中的下一个刻度的定时器分散到256ms盘
            !_Cacsade(m_list3, _Index(1)) && //将17m盘中的下一个刻度的定时器分散到16s盘
            !_Cacsade(m_list4, _Index(2))) //此语句的效果是，只要前面判断有效，则一直执行下去
        {
            _Cacsade(m_list5, _Index(3)); // 3
        }

        m_lastCheckTime.AddDelay(1); // 处理下一个ms

        Timer* timer;
        while ((timer = m_list1[index]->next_))
        {
            KillTimer(timer);
            if (timer->OnTimer()) //检查并执行timer的触发回调
                AddTimer(timer);
            else //不再使用，则回收到定时器池
                freepool_.insert(timer);
        }
    }

    return hasUpdated;
}


void TimerQueue::AddTimer(Timer* timer)
{
    uint32_t diff      =  static_cast<uint32_t>(timer->triggerTime_ - m_lastCheckTime);
    uint64_t trigTime  =  timer->triggerTime_.MilliSeconds();
    Timer* pListHead   = nullptr;

    if ((int32_t)diff < 0) // 比m_lastCheckTime还早，则加入到m_lastCheckTime所在刻度队列
    {
        pListHead = m_list1[m_lastCheckTime.MilliSeconds() & (LIST1_SIZE - 1)];
    }
    else if (diff < static_cast<uint32_t>(LIST1_SIZE))
    {
        pListHead = m_list1[trigTime & (LIST1_SIZE - 1)];
    }
    else if (diff < 1 << (LIST1_BITS + LIST_BITS))
    {
        pListHead = m_list2[(trigTime >> LIST1_BITS) & (LIST_SIZE - 1)];
    }
    else if (diff < 1 << (LIST1_BITS + 2 * LIST_BITS))
    {
        pListHead = m_list3[(trigTime >> (LIST1_BITS + LIST_BITS)) & (LIST_SIZE - 1)];
    }
    else if (diff < 1 << (LIST1_BITS + 3 * LIST_BITS))
    {
        pListHead = m_list4[(trigTime >> (LIST1_BITS + 2 * LIST_BITS)) & (LIST_SIZE - 1)];
    }
    else
    {
        pListHead = m_list5[(trigTime >> (LIST1_BITS + 3 * LIST_BITS)) & (LIST_SIZE - 1)];
    }

    assert(!pListHead->prev_);

    timer->prev_ = pListHead;
    timer->next_ = pListHead->next_;
    if (pListHead->next_)
        pListHead->next_->prev_ = timer;
    pListHead->next_ = timer;

    freepool_.erase(timer);
    timerCnt_++;
}

void TimerQueue::ScheduleAt(Timer* timer, const Time& triggerTime)
{
    if (!timer)
        return;

    timer->triggerTime_ = triggerTime;
    AddTimer(timer);
}

// 只是从所在的双向链表中取出
void TimerQueue::KillTimer(Timer* timer)
{
    if (!timer)
        return;

    if (timer->prev_)
    {
        timer->prev_->next_ = timer->next_;

        if (timer->next_)
        {
            timer->next_->prev_ = timer->prev_;
            timer->next_ = nullptr;
        }

        timer->prev_ = nullptr;
    }

    timerCnt_--;
}

//将大刻度的定时器队列中的某个刻度的定时器分散到更高精度的轮盘中
bool TimerQueue::_Cacsade(Timer* pList[], int index)
{
    assert (pList);

    if (index < 0 ||
        index >= LIST_SIZE)
        return  false;

    assert (pList[index]);

    if (!pList[index]->next_)
        return false;

    Timer* tmpListHead = pList[index]->next_;
    pList[index]->next_ = nullptr;

    while (tmpListHead)
    {
        Timer* next = tmpListHead->next_;

        tmpListHead->next_ = nullptr;
        tmpListHead->prev_ = nullptr;

        AddTimer(tmpListHead); //一般只会分散到更高一级精度的时间轮盘中
        tmpListHead = next;
    }

    return true;
}
