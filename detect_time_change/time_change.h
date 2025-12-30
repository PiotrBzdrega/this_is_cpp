
#include <unistd.h>
#include <sys/timerfd.h>
#include <limits>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <print>
#include <chrono>

class time_change
{
private:
    int fd;

public:
    time_change(/* args */);
    ~time_change();
    int wait();
};

time_change::time_change(/* args */)
{
    struct itimerspec timer{};
    timer.it_value.tv_sec = std::numeric_limits<time_t>::max();



    fd = timerfd_create(CLOCK_REALTIME, 0);
    if (fd == -1)
    {
        std::println("timerfd_create failed: {}", std::strerror(errno));
        return;
    }

    if (timerfd_settime(fd, TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET, &timer, nullptr) == -1)
    {
        std::println("timerfd_settime failed: {}", std::strerror(errno));
        return;
    }
}

time_change::~time_change()
{
    if (fd > 0)
    {
        close(fd);
    }
}

inline int time_change::wait()
{
    uint64_t val;
    int ret;
    ret = read(fd, &val, sizeof(uint64_t));
    std::println("Time has changed! ret={}/{}", ret, ret == -1 ? std::strerror(errno) : "");
    std::println("{}", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
    return 0;
}
