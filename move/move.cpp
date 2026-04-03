#include <cstdio>
#include <string>


/*  Copies of rvalues are generally move constructed, 
while copies of lvalues are usually copy constructed.  */

class widget
{
private:
    int a;
    std::string b;
public:
    widget();
    widget(widget& other);  //copy constructor
    widget(widget&& other); /* noexcept */ //move constructor
    widget& operator=(const widget& other); //copy constructor
    widget& operator=(const widget&& other); //move constructor
    ~widget();
    void setA(int newA);
    void setB(std::string newB);
};


widget::widget()
{
    std::printf("constructor\n");
}

widget::widget(widget &other) : a(other.a), b(other.b)
{
    std::printf("copy constructor\n");
}

widget::widget(widget &&other) : a(std::move(other.a)), b(std::move(other.b))
{
    std::printf("move constructor\n");
}

widget &widget::operator=(const widget &other)
{
    a = other.a;
    b = other.b;
    std::printf("copy assignment\n");
    return *this;
}

widget &widget::operator=(const widget &&other)
{
    a = std::move(other.a);
    b = std::move(other.b);
    std::printf("move assignment\n");
    return *this;
}

widget::~widget()
{
    std::printf("destructor\n");
}

void widget::setA(int newA)
{
    a=newA;
}

void widget::setB(std::string newB)
{
    b=newB;
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






widget function()
{
    widget wid;
    wid.setA(12);
    wid.setB("adam");

    return wid;
}



int main()
{
    auto x = function();
    std::printf("Hello\n");
    return 0;
}

