#include <functional>
#include <print>
#include <future>

int main()
{
    std::packaged_task<double()> packaged_task([]()
                                               { return 2025.3; });
    std::future<double> future = packaged_task.get_future();
    auto lambda = [task = std::move(packaged_task)]() mutable
    { task(); };
    std::move_only_function<void()> function = std::move(lambda);
    function();
    std::println("{}",future.get());
}