#include <iostream>

#include "mock.h"

void test(int i) { std::cout << "test " << i << std::endl; }
int test2(int i) { std::cout << "test2 " << i << std::endl; return i; }
class MyTest {
   public:
    MyTest(int _data) : data(_data){};
    ~MyTest() = default;
    void Print() { std::cout << data << std::endl; }

   private:
    int data;
};

int main(int argc, char** argv) {
    MOCK(test, [](int i) { mock::CallOriginalVoid(0); });
	MOCK(test2, [](int i) -> int { mock::CallOriginal<int>(0); });
    MOCK(&MyTest::Print, [](MyTest* test) {
        std::cout << "xxxx" << std::endl;
        mock::CallOriginalVoid(test);
        std::cout << "----" << std::endl;
    });

    test(20);
	test2(99);

    MyTest test(10);
    test.Print();

    return 0;
}