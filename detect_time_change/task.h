#include <coroutine>
#include <exception>

struct Task
{
    struct promise_type
    {
        int result;
        Task get_return_object()
        {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() { std::terminate(); }

        void return_value(int value)
        {
            result = value;
        }
    };

    std::coroutine_handle<promise_type> coro;

    explicit Task(std::coroutine_handle<promise_type> h) : coro(h) {}
    ~Task()
    {
        if (coro)
            coro.destroy();
    }

    int get_result()
    {
        coro.resume();
        return coro.promise().result;
    }
};