/* std::shared_lock 
multiple threads can hold a std::shared_lock on the same std::shared_mutex simultaneusly
This is useful in scenarios where multiple threads need to read a shared resource
concurrently without modifying it.*/

#include <thread>
#include <shared_mutex>
#include <mutex>
#include <print>
#include <chrono>
#include <vector>
#include <ranges>

using namespace std::chrono_literals;

std::shared_mutex sh_mtx;
uint32_t shared_data{};

void read_data()
{
    std::println("[{}] started reader",std::this_thread::get_id());
    std::shared_lock<std::shared_mutex> lock(sh_mtx);
    std::this_thread::sleep_for(10s);
    std::println("[{1}] read {0}",shared_data,std::this_thread::get_id());
}

void write_data(int val)
{
    std::println("[{}] started writer",std::this_thread::get_id());
    std::unique_lock<std::shared_mutex> lock(sh_mtx);
    std::this_thread::sleep_for(5s);
    shared_data = val;
    std::println("[{1}] write {0}",shared_data,std::this_thread::get_id());
}

int main()
{
    std::vector<std::jthread> readers;
    std::vector<std::jthread> writers;
    for (int i : std::views::iota(1, 7))
    {
        readers.emplace_back(std::jthread(write_data,i));
        readers.emplace_back(std::jthread(read_data));
    }
    
    std::println("hello world");
}