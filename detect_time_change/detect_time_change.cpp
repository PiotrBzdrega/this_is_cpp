
#include "time_change.h"
#include "task.h"

Task compute_square(int x)
{
    co_return x *x; // Returns a value
}

int main(int argc, char *argv[])
{

    // auto tm = time_change();
    // while (1)
    // {
    //     tm.wait();
    // }

    auto task = compute_square(5);
    std::println("Square of 5 is: {}",task.get_result());
}