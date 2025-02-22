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

// TODO: Concurrency in Action  Appendix A.1 Rvalue references 1.Move semantics 357

/*  For class X the move constructor is an optimization, but in some cases it makes
              
sense to provide a move constructor even when it doesn’t make sense to provide a
           
copy constructor. For example, the whole point of std::unique_ptr<> is that each
              
non-null instance is the one and only pointer to its object, so a copy constructor
              
makes no sense. But a move constructor allows ownership of the pointer to be trans
     
      
ferred between instances and permits std::unique_ptr<> to be used as a function
       
return value—the pointer is moved rather than copied
      
X x1;
X x2=std::move(x1);
X x3=static_cast<X&&>(x2);

 */










int main()
{
    std::printf("Hello\n");
    return 0;
}

