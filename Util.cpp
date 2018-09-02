#include "Util.h"
#include <stdarg.h>
#include <memory>
#include <algorithm>
#include <chrono>
#include <fcntl.h>

int util::addFdFlag(int fd, int flag) {
    int ret = fcntl(fd, F_GETFD);
    return fcntl(fd, F_SETFD, ret | flag);
}

int64_t util::timeMicro() {
    std::chrono::time_point<std::chrono::system_clock> p = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(p.time_since_epoch()).count();
}