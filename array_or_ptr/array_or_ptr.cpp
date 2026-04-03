#include <cstdio>
#include <string>



void print_array(const char param[])
{
    std::printf("%s\n",param);
}

void print_ptr(const char *param)
{
    std::printf("%s\n",param);
}

int main()
{
    std::printf("build file: %s\n%s\n%s\n", __FILE__,__TIME__,__DATE__);
    std::printf("####################\n\n");


    const char name[] = "J. P. Briggs";
    const char * ptrToName = name; // array decays to pointer to const char
    print_array(name);
    print_array(ptrToName);

    print_ptr(name);
    print_ptr(ptrToName);

    return 0;
}