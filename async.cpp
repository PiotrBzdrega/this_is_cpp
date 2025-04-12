#include <future>
#include <print>
#include <chrono>

using namespace std::chrono;

std::mutex m;

void foo(int counter)
{
    std::lock_guard<std::mutex> lk(m);
    for(int i =0; i<counter; ++i)
    {
        // std::this_thread::sleep_for(seconds(10));
        std::this_thread::sleep_until(system_clock::now()+seconds(10));
        std::println("[{}] {} date:{}",__FUNCTION__,i,system_clock::now());
    }
}

void bar(std::string_view str)
{
    std::lock_guard<std::mutex> lk(m);
    std::println("[{}] {}",__FUNCTION__,str);
}


int main()
{
    //std::launch::async each task runs in its own thread
    auto task1 = std::async(std::launch::async, foo,5);
    auto task2 = std::async(std::launch::async, bar,"adam");

    task2.wait();
    std::println("[{}] task2 available",__FUNCTION__);
    task1.wait();

    return 0;
}