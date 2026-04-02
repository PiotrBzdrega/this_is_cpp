/**
 * PROBABLY NOT FINISHED
 * FINAL WORKING IMPLEMENTATION
 * IN PAYLINK
 */
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
using clock_ms = std::chrono::time_point<std::chrono::steady_clock, std::chrono::milliseconds>;

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
        next_time_call = std::chrono::floor<std::chrono::milliseconds>(std::chrono::steady_clock::now()) + interval;
    }

public:
    explicit task(task_t &&t_, std::chrono::milliseconds interval_, std::size_t id_, repeat_t repeat_, std::chrono::seconds startup_delay_ = 0s) : t(std::move(t_)),
                                                                                                                                                   interval(interval_), id(id_),
                                                                                                                                                   next_time_call(std::chrono::floor<std::chrono::milliseconds>(std::chrono::steady_clock::now()) - interval_ + startup_delay_),
                                                                                                                                                   repeat(repeat_) {}

    auto get_next_time_call() const { return next_time_call; }
    auto get_id() const { return id; }
    auto get_interval() const { return interval; }
    auto get_repeat() const { return repeat; }
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
    std::vector<task> tasks;
    std::mutex mtx;
    std::condition_variable cv;
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
    std::size_t submit_periodic_task(task_t &&t_, std::chrono::milliseconds interval_, uint32_t repeat_= 0)
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
    auto lambda_id = sched.submit_periodic_task(lambda, 1000ms);
    std::string some_string{"no-value"};
    sched.submit_periodic_task([&some_string]()
                               {  auto now =  std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
                                std::println("set {}", now); some_string = std::format("{}",now); }, 500ms);
    sched.submit_periodic_task([&some_string]()
                               { std::println("get {}", some_string); }, 700ms);                            

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
            cv.wait_until(lck, task_in_sight.available() ? tasks[task_in_sight.index].get_next_time_call() : std::chrono::steady_clock::time_point::max(), [&]
                          { return st.stop_requested() || periodic_tasks_size != tasks.size(); });
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

            if (task_in_sight.available())
            {
                if (tasks.size() - 1 >= task_in_sight.index &&
                    task_in_sight.id == tasks[task_in_sight.index].get_id())
                {
                    auto &t = tasks[task_in_sight.index];
                    auto now = std::chrono::floor<std::chrono::milliseconds>(std::chrono::steady_clock::now());

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

    std::println("worker finished");
}

scheduler::scheduler()
{
    thr_worker = std::jthread(std::bind_front(&scheduler::worker, this));
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
}