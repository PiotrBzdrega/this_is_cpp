#include <cstdio>
#include <string>

constexpr auto  lambda1 =[](){return "lambda1";};
constexpr auto  lambda2 =[](auto x){return x+1;};








int main()
{
    std::printf("build file: %s\n%s\n%s\n", __FILE__,__TIME__,__DATE__);
    std::printf("####################\n\n");
    std::printf("%s\n",[](){return "immediatelly called lambda";}());
    std::printf("%s\n",lambda1());

    /* auto parameter */
    auto res_lambda2a = lambda2(14);
    auto res_lambda2b = lambda2(14.41);
    auto res_lambda2c = lambda2("123");
    std::printf("int:%d\nfloat:%f\nconst char*:%s\n",res_lambda2a,res_lambda2b,res_lambda2c);

    /* capture */
    int a=13;
    // auto  lambda3a = [=](){a++;}; //[ERROR] not modifiable lvalue
    auto  lambda3b = [&](){a++;};
    
    std::printf("before lambda capture by ref:%d \n",a);
    lambda3b();
    std::printf("after lambda capture by ref:%d\n",a);
    
    return 0;
}