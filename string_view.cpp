#include <cstdio>
#include <string>


void string_view_value(std::string_view str)
{
    std::printf("%s\n",str.data());
}


int main()
{
    std::printf("build file: %s\n%s\n%s\n", __FILE__,__TIME__,__DATE__);
    std::printf("####################\n\n");
    std::string string{"adamek"};
    string_view_value(string);
    return 0;
}