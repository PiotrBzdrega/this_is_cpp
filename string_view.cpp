#include <cstdio>
#include <string>
#include <print>

void string_view_value(std::string_view str)
{
    std::printf("%s\n",str.data());
}

struct string_view_in_cstr
{
    std::string_view _view;
    string_view_in_cstr(std::string_view view) : _view(view)
    {

    }

    void print_view()
    {
        std::println("{}",_view);
    }
};

int main()
{
    std::printf("build file: %s\n%s\n%s\n", __FILE__,__TIME__,__DATE__);
    std::printf("####################\n\n");
    std::string string{"adamek"};
    string_view_value(string);
    auto instance = string_view_in_cstr("abc");
    instance.print_view();

    return 0;
}