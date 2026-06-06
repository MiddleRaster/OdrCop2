
#include <iostream>

struct SomeStruct
{
    int aDataMember = 0;
    SomeStruct() {}
   ~SomeStruct() {}
};

int main()
{
    std::cout << "Hello World!\n";
    return SomeStruct().aDataMember;
}
