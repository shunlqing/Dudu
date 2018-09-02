#ifndef DUDU_UTIL_H
#define DUDU_UTIL_H

#include <functional>

typedef std::function<void ()> Task;

struct noncopyable {
    noncopyable() {}
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
};

struct util {
    static int64_t timeMicro();
    static int64_t timeMilli() { return timeMicro()/1000; }

    int addFdFlag(int fd, int flag);
};



#endif //DUDU_UTIL_H
