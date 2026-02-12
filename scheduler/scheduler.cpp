#include <print>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <functional>
#include <optional>

using namespace std::chrono_literals;
using task_t = std::move_only_function<void()>;
using clock_ms = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>;

class periodic_task
{
    // TODO: execute periodic task given number of times
private:
    task_t task;
    std::chrono::milliseconds interval;
    clock_ms next_time_call{};
    void specify_future_call()
    {
        next_time_call = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now()) + interval;
    }

public:
    explicit periodic_task(task_t &&task_, std::chrono::milliseconds interval_) : task(std::move(task_)), interval(interval_) {}
    auto get_next_time_call() { return next_time_call; }
    void operator()()
    {
        task();
        specify_future_call();
    }
};

class scheduler
{
private:
    std::jthread thr_work;
    std::vector<periodic_task> tasks;
    void worker(std::stop_token st);
    std::mutex mtx;
    std::condition_variable cv;
    std::optional<task_t> one_shot_task;

public:
    scheduler(/* args */);
    ~scheduler();
    void submit_periodic_task(task_t &&task_, std::chrono::milliseconds interval_)
    {
        {
            std::lock_guard<std::mutex> lck(mtx);
            tasks.emplace_back(std::move(task_), interval_);
        }
        cv.notify_one();
    }
    void submit_task(task_t &&task_)
    {
        {
            std::lock_guard<std::mutex> lck(mtx);
            one_shot_task = std::move(task_);
        }
        cv.notify_one();
    }
};

int main()
{
    scheduler sched;
    auto lambda = []()
    {
        std::println("adamek");
    };
    sched.submit_periodic_task(lambda, 1000ms);
    sched.submit_periodic_task([]()
                               { std::println("{}", std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now())); }, 500ms);

    std::this_thread::sleep_for(3s);

    sched.submit_task([]()
                      { std::println("on demand function"); });

    std::this_thread::sleep_for(5s);
}

void scheduler::worker(std::stop_token st)
{
    periodic_task *task_in_sight{};
    bool wait_pred{};
    while (!st.stop_requested())
    {

        // TODO: do not search for a new periodic task if is old still ready, just compare if container size is the same like last
        /*
        if (!task_in_sight)
        {
        }
        */

        // TODO: handle also periodic task removal, remove can be executed under lock

        auto periodic_tasks_size = [&]()
        {
            /* lock function scope */
            std::lock_guard<std::mutex> lck(mtx);

            /* Search for closest future task to execute */
            for (auto &task : tasks)
            {
                /* take first task in container */
                if (!task_in_sight)
                {
                    task_in_sight = &task;
                }
                else
                {
                    /* task should be executed earlier than task_in_sight */
                    if (task.get_next_time_call() < task_in_sight->get_next_time_call())
                    {
                        task_in_sight = &task;
                    }
                }
            }
            /* Store current task size to detect if new appeared in container */
            return tasks.size();
        }();

        // TODO: detect system time change
        {
            std::unique_lock<std::mutex> lck(mtx);
            /* Wait until stop token requested, new one shot or periodic task appeared or timeout that imply need to execute next periodic task */
            cv.wait_until(lck, task_in_sight ? task_in_sight->get_next_time_call() : std::chrono::system_clock::time_point::max(), [&]
                          { return st.stop_requested() || one_shot_task || periodic_tasks_size != tasks.size(); });
        }

        if (st.stop_requested())
        {
            return;
        }

        // TODO: handle one shot task future to pass to caller
        /* New one shot task availble */
        if (one_shot_task)
        {
            /* Call one shot task */
            one_shot_task.value()();

            /* Remove task */
            one_shot_task.reset();
        }

        auto now = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());

        /* Periodic task ready to execute and interval expired */
        if (task_in_sight && task_in_sight->get_next_time_call() <= now)
        {
            /* Execute task */
            task_in_sight->operator()();

            /* Drop task */
            task_in_sight = nullptr;
        }
    }
}

scheduler::scheduler()
{
    thr_work = std::jthread(std::bind_front(&scheduler::worker, this));
}

scheduler::~scheduler()
{
    if (thr_work.joinable())
    {
        thr_work.request_stop();
        thr_work.join();
    }
}