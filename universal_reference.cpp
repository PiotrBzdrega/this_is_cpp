#include <cstdio>
#include <string>
#include <typeinfo>
#include <type_traits>
#include "cxxabi.h"

template <typename T>
void uni_ref(T &&x)
{

    std::printf("type:%s=%d\t",typeid(x).name(),x);


    if (std::is_reference_v<decltype(x)>)
    {
        if(std::is_rvalue_reference<decltype(x)>::value)
        {
            std::printf("rvalue ");
        }
        else
        if(std::is_lvalue_reference<decltype(x)>::value)
        {
            std::printf("lvalue ");
        }
        std::printf("reference");
    }

    int status;
    auto realname = abi::__cxa_demangle(typeid(x).name(), nullptr, nullptr, &status);


    if (std::is_same_v<T&&, decltype(x)>)
    {
        std::printf("\t%s&& ",realname);
    }


    if (std::is_same_v<T&, decltype(x)>)
    {
        std::printf("\t%s& ",realname);
    }

    if (std::is_same_v<T, decltype(x)>)
    {
        std::printf("\t%s ",realname);
    }

    std::printf("\n");
}

int main()
{
    std::printf("build file: %s\n%s\n%s\n", __FILE__,__TIME__,__DATE__);
    std::printf("####################\n\n");

    auto&& r1 = 13;
    uni_ref(13);

    int x = 14;
    auto&& r2= x;
    uni_ref(x);

    int y = 15;
    auto &&r3 = y;
    uni_ref(y);

    return 0;
}