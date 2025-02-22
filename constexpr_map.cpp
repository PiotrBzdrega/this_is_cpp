#include <cstdio>
#include <string>
#include <array>


constexpr void myCallback(std::string &a) {
    std::printf("####################\n\n");
    a+=std::string("rest");
}

constexpr void myCallback2(std::string &a) {
    a=std::string("myCallback2");
}

template <typename func>
struct value
{
    value() = default;
    constexpr value(int a,std::string_view b, func c) : a(a),b(b),c(c){[this](){}/* syslog(LOG_NOTICE, "Constructor!!");}(); */; }
    
private:
    int a;
public:
    std::string_view b;
    func c;
};

using ValueType = value<void (*)(std::string&)>;

constexpr std::pair<std::string_view, ValueType> data[] =
{
    {"adam",value{1,"adam",myCallback}},
    {"ewa",value{1,"ewa",myCallback2}}
};

constexpr auto value_commands = std::to_array(data); // Deduces size automatically


void constexpr_map()
{}





int main()
{
    std::printf("build file: %s\n%s\n%s\n", __FILE__,__TIME__,__DATE__);
    std::printf("####################\n\n");

    auto x = value_commands[0];
    std::printf("%s\n",x.first.data());

    std::string string{"adamek"};
    x.second.c(string);
    std::printf("%s\n",string.c_str());
    return 0;
}