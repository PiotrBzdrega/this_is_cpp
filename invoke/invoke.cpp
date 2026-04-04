#include <print>
#include <functional>

// Function pointer type for the callback (C-compatible)
typedef void (*CallbackFunc)(const char* message, int value);

void callback(const char *msg, int value)
{
    std::println("obtained callback with values\n\
        msg: {}\n\
        value: {}",
                 msg, value);
}


int main()
{
    std::println("hello from invoke");
    CallbackFunc cb = &callback;
    cb("invoke calling back", 42);

    std::invoke(cb, "invoke calling back via std::invoke", 84);
    return 0;
}