#include <print>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <functional>
#include <optional>
#include <ranges>
#include <future>

using namespace std::chrono_literals;
using task_t = std::move_only_function<void()>;
using clock_ms = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>;

class periodic_task
{
private:
    task_t task;
    std::chrono::milliseconds interval;
    std::size_t id;
    // TODO: execute periodic task given number of times
    int call_counter{-1};
    clock_ms next_time_call{};
    void specify_future_call()
    {
        next_time_call = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now()) + interval;
    }

public:
    explicit periodic_task(task_t &&task_, std::chrono::milliseconds interval_, std::size_t id_) : task(std::move(task_)), interval(interval_), id(id_) {}
    auto get_next_time_call() const { return next_time_call; }
    auto get_id() const { return id; }
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
    std::mutex mtx;
    std::condition_variable cv;
    std::optional<task_t> one_shot_task;
    std::uint16_t counter;
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

public:
    scheduler(/* args */);
    ~scheduler();
    std::size_t submit_periodic_task(task_t &&task_, std::chrono::milliseconds interval_)
    {
        auto id = std::hash<std::string_view>{}(std::to_string(counter++));
        {
            std::println("new task {}", id);
            std::lock_guard<std::mutex> lck(mtx);
            tasks.emplace_back(std::move(task_), interval_, id);
        }
        cv.notify_one();
        return id;
    }

    bool remove_periodic_task(std::size_t id_)
    {
        bool res{};
        {
            std::lock_guard<std::mutex> lck(mtx);
            for (const auto &[task_i, task_ref] : std::views::enumerate(tasks))
            {
                if (task_ref.get_id() == id_)
                {
                    std::println("removed task {}", id_);
                    tasks.erase(tasks.begin() + task_i);
                    res = true;
                    break;
                }
            }
        }
        // TODO: not sure if i should notify condition variable if tasks container is untouched
        cv.notify_one();
        return res;
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
    auto lambda_id = sched.submit_periodic_task(lambda, 1000ms);
    sched.submit_periodic_task([]()
                               { std::println("{}", std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now())); }, 500ms);

    std::this_thread::sleep_for(3s);
    sched.remove_periodic_task(lambda_id);

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
    sched.submit_periodic_task(lambda, 2s);
    std::println("returned future {}", fut.get());
    std::this_thread::sleep_for(15s);
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

            /* Wait until stop token requested, new one shot or periodic task appeared or timeout that imply need to execute next periodic task */
            cv.wait_until(lck, task_in_sight.available() ? tasks[task_in_sight.index].get_next_time_call() : std::chrono::system_clock::time_point::max(), [&]
                          { return st.stop_requested() || one_shot_task || periodic_tasks_size != tasks.size(); });
        }

        // TODO: detect system time change, how much did it change, correct next time call for all tasks, if it is not possible -> store last finished execution and future call ,
        // wait difference between those two, if current time is not between make adjustments to all periodic tasks
        {
        }

        if (st.stop_requested())
        {
            return;
        }

        /* New one shot task availble */
        if (one_shot_task)
        {
            /* Call one shot task */
            one_shot_task.value()();

            /* Remove task */
            one_shot_task.reset();
        }

        {
            /* lock function scope */
            std::lock_guard<std::mutex> lck(mtx);
            auto now = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());

            if (task_in_sight.available())
            {
                if (tasks.size() - 1 >= task_in_sight.index &&
                    task_in_sight.id == tasks[task_in_sight.index].get_id())
                {
                    if (tasks[task_in_sight.index].get_next_time_call() <= now)
                    {
                        // TODO: make sure that we need to have lock during task execution
                        /* Execute task */
                        tasks[task_in_sight.index]();

                        // TODO: increment/decrement call_counter here

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