#include <print>
#include <optional>
#include <expected>

std::optional<double> divide(int a, int b)
{
    if (b == 0)
    {
        return std::nullopt;
    }
    return static_cast<double>(a) / static_cast<double>(b);
}

int main()
{
    auto result = std::optional<int>{12}
                      .and_then([](int x)
                                { return divide(x, 5); })
                      .or_else([]()
                               { return std::optional<double>{0.0}; });

    if (result)
    {
        std::println("monadic {}", result.value());
    }
}