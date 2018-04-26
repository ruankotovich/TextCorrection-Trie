#include <string>

struct Person {
    std::string name;
    int age;

    Person(int _age, const std::string& _name)
        : name(_name)
        , age(_age)
    {
    }
    Person()
    {
    }
    bool operator()(const Person& p1, const Person& p2)
    {
        return p1.age < p2.age;
    }
};