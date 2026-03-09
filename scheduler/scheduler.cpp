#include <print>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <functional>
#include <optional>
#include <ranges>
#include <future>
#include <variant>
#include <unistd.h>
#include <sys/timerfd.h>
#include <cstring>
#include <sys/eventfd.h>
#include <poll.h>

using namespace std::chrono_literals;
using task_t = std::move_only_function<void()>;
using clock_ms = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>;

using repeat_t = std::optional<uint32_t>;

class task
{
private:
    task_t t;
    std::chrono::milliseconds interval;
    std::size_t id;
    // TODO: execute periodic task given number of times
    /*  nullopt - (-1), once - 0, many - x>0, */
    repeat_t repeat;
    clock_ms next_time_call{};
    void specify_future_call()
    {
        next_time_call = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now()) + interval;
    }

public:
    explicit task(task_t &&t_, std::chrono::milliseconds interval_, std::size_t id_, repeat_t repeat_, std::chrono::seconds startup_delay_ = 0s) : t(std::move(t_)),
                                                                                                                                                   interval(interval_), id(id_),
                                                                                                                                                   next_time_call(std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now()) - interval_ + startup_delay_),
                                                                                                                                                   repeat(repeat_) {}

    auto get_next_time_call() const { return next_time_call; }
    auto get_id() const { return id; }
    auto get_interval() const { return interval; }
    auto get_repeat() const { return repeat; }
    void specify_future_call(clock_ms &next_time_call_)
    {
        next_time_call = next_time_call_;
    }
    /**
     * @brief execute task, then override next future time call
     * @return if repeat counter reached zero
     */
    bool operator()()
    {
        t();
        if (repeat)
        {
            auto val = repeat.value();
            if (val > 0)
            {
                repeat = --val;
            }
            else
            {
                return true;
            }
        }
        specify_future_call();
        return false;
    }
};

class scheduler
{
private:
    std::jthread thr_worker;
    std::jthread thr_time_change;
    std::vector<task> tasks;
    std::mutex mtx;
    std::condition_variable cv;
    std::uint16_t counter;
    bool time_change_detected{};
    struct task_in_sight_t
    {
        int index{-1};
        std::size_t id{};
        void reset()
        {
            index = -1;
            id = 0;
        }
        bool available() const { return index != -1; }
    } task_in_sight;
    void worker(std::stop_token st);
    void time_change(std::stop_token st);
    bool remove_task_(std::size_t id_)
    {
        for (const auto &[task_i, task_ref] : std::views::enumerate(tasks))
        {
            if (task_ref.get_id() == id_)
            {
                std::println("removed task {}", id_);
                tasks.erase(tasks.begin() + task_i);
                return true;
            }
        }
        return false;
    }

public:
    scheduler(/* args */);
    ~scheduler();
    std::size_t submit_periodic_task(task_t &&t_, std::chrono::milliseconds interval_, uint32_t repeat_)
    {
        repeat_t r = repeat_ > 0 ? repeat_t{repeat_ - 1} : std::nullopt;
        auto id = std::hash<std::string_view>{}(std::to_string(counter++));
        {
            std::println("new task {}", id);
            std::lock_guard<std::mutex> lck(mtx);
            tasks.emplace_back(std::move(t_), interval_, id, r);
        }
        cv.notify_one();
        return id;
    }

    bool remove_task(std::size_t id_)
    {
        bool res{};
        {
            std::lock_guard<std::mutex> lck(mtx);
            res = remove_task_(id_);
        }
        // TODO: not sure if i should notify condition variable if tasks container is untouched
        cv.notify_one();
        return res;
    }

    // TODO: not sure if inserting one shot tasks to vector and immediatelly remove it is good idea ?? maybe one task is fine idea
    //  start with one vector for periodic and one shot, test and change for separeted one shot struct if won't be efficient
    std::size_t submit_task(task_t &&t_)
    {
        return submit_periodic_task(std::move(t_), 0ms, 1);
    }
};

int main()
{
    scheduler sched;
    auto lambda = []()
    {
        std::println("adamek");
    };
    auto lambda_id = sched.submit_periodic_task(lambda, 1000ms, 0);
    sched.submit_periodic_task([]()
                               { std::println("{}", std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now())); }, 500ms, 0);

    std::this_thread::sleep_for(3s);
    sched.remove_task(lambda_id);

    auto prom = std::promise<std::string>{};
    auto fut = prom.get_future();
    /*
    std::promise::set_value() is non-const - it modifies the promise's state
    By default, lambda's operator() is const - all captured by-value variables are const inside
    mutable removes the const from the lambda's operator()
    */
    sched.submit_task([prom = std::move(prom)]() mutable
                      { std::println("on demand function start");
                         std::this_thread::sleep_for(1s);
                          std::println("on demand function finished");
                        prom.set_value("lala finished"); });

    std::this_thread::sleep_for(3s);
    sched.submit_periodic_task(lambda, 2s, 2);
    std::println("returned future {}", fut.get());
    std::this_thread::sleep_for(8s);
}

void scheduler::worker(std::stop_token st)
{

    bool wait_pred{};
    while (!st.stop_requested())
    {

        // TODO: do not search for a new periodic task if is old still ready, just compare if container size is the same like last
        /*
        if (!task_in_sight)
        {
        }
        */

        // TODO: assure task cancelation, for graceful class shutdown (maybe each periodic task should have cancel() function, or wrap given function to kill it anytime (if possible))
        {
            std::unique_lock<std::mutex> lck(mtx);

            /* Search for closest future task to execute */
            for (const auto &[task_i, task_ref] : std::views::enumerate(tasks))
            {
                /* take first task in container */
                if (!task_in_sight.available())
                {
                    task_in_sight = {static_cast<int>(task_i), task_ref.get_id()};
                }
                else
                {
                    /* task should be executed earlier than task_in_sight */
                    if (task_ref.get_next_time_call() < tasks[task_in_sight.index].get_next_time_call())
                    {
                        task_in_sight = {static_cast<int>(task_i), task_ref.get_id()};
                    }
                }
            }
            /* Store current task size to detect if new appeared in container */
            auto periodic_tasks_size = tasks.size();

            /* Wait until stop token requested, periodic task appeared or timeout that imply need to execute next periodic task */
            cv.wait_until(lck, task_in_sight.available() ? tasks[task_in_sight.index].get_next_time_call() : std::chrono::system_clock::time_point::max(), [&]
                          { return st.stop_requested() || periodic_tasks_size != tasks.size() || time_change_detected; });
        }

        // TODO: detect system time change, how much did it change, correct next time call for all tasks, if it is not possible -> store last finished execution and future call ,
        // wait difference between those two, if current time is not between make adjustments to all periodic tasks interval

        if (st.stop_requested())
        {
            break;
        }

        {
            /* lock function scope */
            std::lock_guard<std::mutex> lck(mtx);

            /* system time has been modified */
            if (time_change_detected)
            {
                time_change_detected = false;
                auto now = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());

                /* loop through all periodic tasks */
                for (auto &t : tasks)
                {
                    auto last_call = t.get_next_time_call() - t.get_interval();
                    auto next_call = t.get_next_time_call();

                    /*  */
                    if (auto backward_diff = last_call - now; backward_diff > 0ms)
                    {
                        /* When last_call == now -> do nix, next call is set with interval */
                        ;

                        /* (now) < (last_call) < (next_call) */
                        /* (now) <= (new_next_call) <= (last_call)*/
                        /* Find next slot to fits "every x clock units" for task execution */
                        /* If backward_diff < task interval -> it will be set to last_call */
                        auto new_next_call = now + (backward_diff % t.get_interval());
                        t.specify_future_call(new_next_call);
                    }
                    else if (auto forward_diff = now - next_call; forward_diff > 0ms)
                    {
                        /* When next_call == now -> do nix, next call is already set to correct slot */
                        ;

                        /* (last_call) < (next_call) < (now) */
                        /* (next_call) <= (new_next_call) <= (now) */
                        /* Find next slot to fits "every x clock units" for task execution */
                        /* If forward_diff < task interval -> set next call in difference of these two values */
                        auto new_next_call = now + ((forward_diff < t.get_interval()) ? t.get_interval() - forward_diff : forward_diff % t.get_interval());
                        t.specify_future_call(new_next_call);
                    }
                }

                continue;
            }

            if (task_in_sight.available())
            {
                if (tasks.size() - 1 >= task_in_sight.index &&
                    task_in_sight.id == tasks[task_in_sight.index].get_id())
                {
                    auto &t = tasks[task_in_sight.index];
                    auto now = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());

                    if (t.get_next_time_call() <= now)
                    {
                        // TODO: make sure that we need to have lock during task execution
                        /* Execute task */
                        if (t())
                        {
                            remove_task_(task_in_sight.id);
                        }

                        /* Drop task */
                        task_in_sight.reset();
                    }
                }
                else
                {
                    /* Task available but not in the same place as before, probably removed or relocated */
                    /* Drop task */
                    task_in_sight.reset();
                }
            }
        }
    }
}

void scheduler::time_change(std::stop_token st)
{
    struct itimerspec timer{};
    timer.it_value.tv_sec = std::numeric_limits<time_t>::max();
    uint64_t val;
    int ret;

    int timer_fd = timerfd_create(CLOCK_REALTIME, 0);
    int stop_fd = eventfd(0, 0);
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

    std::stop_callback cb(st, [&stop_fd]
                          {    
        if (stop_fd >= 0)
        {
            uint64_t v = 1;
            write(stop_fd, &v, sizeof(v));
        } });

    struct pollfd fds[2]{
        {timer_fd, POLLIN, 0},
        {stop_fd, POLLIN, 0}};

    while (!st.stop_requested())
    {
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
            auto r = read(timer_fd, &val, sizeof(val));

            std::println("Time has changed! ret={}/{}", r, r == -1 ? std::strerror(errno) : "");
            std::println("{}", std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
            {
                std::lock_guard<std::mutex> lck(mtx);
                time_change_detected = true;
            }
            cv.notify_one();
        }
    }
    close(timer_fd);
    close(stop_fd);
}

scheduler::scheduler()
{
    thr_worker = std::jthread(std::bind_front(&scheduler::worker, this));
    thr_time_change = std::jthread(std::bind_front(&scheduler::time_change, this));
    /* thr_time_change = std::jthread([this, &network_config](std::stop_token stoken)
                                      { this->serverHandler(stoken, network_config); }); */
}

scheduler::~scheduler()
{
    if (thr_worker.joinable())
    {
        thr_worker.request_stop();
        thr_worker.join();
    }

    if (thr_time_change.joinable())
    {
        thr_time_change.request_stop();
        thr_time_change.join();
    }
}