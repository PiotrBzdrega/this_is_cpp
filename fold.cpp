#include <cstdio>
#include <string>


/* https://www.foonathan.net/2020/05/fold-tricks/#content */
/* https://breese.github.io/2024/08/11/variadic-fold.html */

template<typename U,typename... T>
std::string fold(U,const T&... params)
{
    std::printf("Print all params:\n");
    (std::printf("%s\n",params), ...);


    std::printf("\n(sizeof...) Queries the number of elements in a parameter pack:\n");
    const int size = sizeof...(params);
    std::printf("%d\n",size);

    std::string_view arr[size]{params...};
    
    std::printf("\nConstructed array out of params:\n"); 
    for (auto const &par : arr)
    {
        std::printf("%s\n",par.data());
    }
    

    std::printf("\nReverse order:\n");    
    int dummy;
    (dummy = ... = (std::printf("%s\n",params), 0));
    


    // return std::string(arr[U].data());
    return std::string("ass\n");
}



int main()
{
    std::printf("build file: %s\n%s\n%s\n", __FILE__,__TIME__,__DATE__);
    std::printf("####################\n\n");
    std::printf("%s",fold(2,"ewa","adam","krzych").c_str());

    return 0;
}
