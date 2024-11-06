#include <cstdio>
#include <string>


/*  Copies of rvalues are generally move constructed, 
while copies of lvalues are usually copy constructed.  */

class widget
{
private:
    int a;
    std::string_view b;
public:
    widget(widget& widget_cp);  //copy constructor
    widget(widget&& widget_mv); //move constructor
    ~widget();
};

// widget::move(/* args */)
// {
// }

widget::widget(widget &move_copy)
{
}

widget::~widget()
{
}












int main()
{
    std::printf("Hello\n");
    return 0;
}

