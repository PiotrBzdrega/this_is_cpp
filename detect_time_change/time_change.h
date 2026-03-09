
#include <unistd.h>
#include <sys/timerfd.h>
#include <limits>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <print>
#include <chrono>
#include <thread>
#include <functional>
#include <sys/eventfd.h>
#include <poll.h>

class time_change
{
private:
    int timer_fd{-1};
    int stop_fd{-1};
    int wait();
    void stop_time_change();
    std::jthread thr;

public:
    time_change(/* args */);
    ~time_change();
};

time_change::time_change(/* args */)
{
    struct itimerspec timer{};
    timer.it_value.tv_sec = std::numeric_limits<time_t>::max();

    timer_fd = timerfd_create(CLOCK_REALTIME, 0);
    stop_fd = eventfd(0, 0);
    if (timer_fd == -1 || stop_fd == -1)
    {
        std::println("timerfd_create failed: {}", std::strerror(errno));
        return;
    }

    if (timerfd_settime(timer_fd, TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET, &timer, nullptr) == -1)
    {
        std::println("timerfd_settime failed: {}", std::strerror(errno));
        return;
    }
    thr = std::jthread(std::bind_front(&time_change::wait, this));
}

time_change::~time_change()
{
    std::println("time_change destructor");

    stop_time_change();

    if (thr.joinable())
    {
        thr.join();
    }

    if (timer_fd > 0)
    {
        close(timer_fd);
    }

    if (stop_fd > 0)
    {
        close(stop_fd);
    }

}

inline int time_change::wait()
{
    uint64_t val;
    struct pollfd fds[2]{
        {timer_fd, POLLIN, 0},
        {stop_fd, POLLIN, 0}};

    int ret = poll(fds, 2, -1);

    if (ret < 0)
    {
        std::println("poll error: {}", std::strerror(errno));
    }

    if (fds[1].revents & POLLIN)
    {
        uint64_t v;
        read(stop_fd, &v, sizeof(v));
        std::println("stop event received");
    }

    if (fds[0].revents & POLLIN)
    {
        uint64_t val;
        read(timer_fd, &val, sizeof(val));

        std::println("timer event");
        std::println("{}", std::chrono::floor<std::chrono::seconds>(
                               std::chrono::system_clock::now()));
    }

    std::println("time_change stop read");
    std::println("Time has changed! ret={}/{}", ret, ret == -1 ? std::strerror(errno) : "");
    std::println("{}", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
    return 0;
}

inline void time_change::stop_time_change()
{
    if (stop_fd >= 0)
    {
        uint64_t v = 1;
        write(stop_fd, &v, sizeof(v));
    }
}
