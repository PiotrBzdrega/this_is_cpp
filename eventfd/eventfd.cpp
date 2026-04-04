/**
 * TODO: Make sure that readable will be signaled even if write() has been notified before FD_SET for select
 */
#include "Worker.h"
int main()
{
    Worker w;
    w.notify();

    std::this_thread::sleep_for(std::chrono::seconds(5));

    w.run();
    std::this_thread::sleep_for(std::chrono::seconds(5));
}