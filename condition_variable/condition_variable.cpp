#include <mutex>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <print>

using namespace std::chrono_literals;

std::mutex mtx;
std::condition_variable cv;
bool resource_ready = false;

void funcA()
{
    std::scoped_lock abc(mtx);
    std::println("enters funcA");

    std::this_thread::sleep_for(4000ms);
    std::println("leave funcA");
}

void funcB()
{
    std::scoped_lock abc(mtx);
    std::println("enters funcB");

    std::println("leave funcB");
}

int main()
{
    funcA();
    std::jthread b(funcB);
}