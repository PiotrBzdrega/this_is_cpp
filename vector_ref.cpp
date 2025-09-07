
#include <print>
#include <vector>
#include <string>

/* 
Q1. I wanted to check if reference to struct element within vector remains usable after vector itself goes out of scope?
A1. It is undefined behavior -sometimes it may work, sometimes it may not. I should return by value instead of by reference

Q2. What yields vector.back() if container is empty?
Q2. exception 🥲
*/

struct some_struct
{
    std::string name;
    int number;
    std::string to_string()
    {
        return std::format("{}:{}",name,number);
    }
};


some_struct &yieldsReferenceToVector()
{
    std::vector<some_struct> vec{{"adam",15},{"ewa",12},{"mewa",1}};
    return vec.back();
}

int main()
{
    std::println("hello world");
    auto& ref_element = yieldsReferenceToVector();
    std::println("{}",ref_element.to_string());

    std::vector<some_struct> local_vec{};

    auto empty_element = local_vec.back();
    std::println("{}",empty_element.to_string());
}