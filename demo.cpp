#include <random>
#include "mock.h"


int test(int i) { return i; }

int test2(int i) { return i * i; }

int test3(int i) { return i + 100; }

class TestClass {
  public:
    TestClass() = default;
    virtual ~TestClass() = default;
    int F1(int i) { return i; }
    bool F2(int i) { return i > 10; }

    virtual bool V1(int i) { return i > 0; }
    virtual int V2(int i) { return i + 1; }
};

int main(int argc, char **argv) {
    MOCK(test, [](int i) -> int { return i + 1; });
    g_assert(test(10) == 11);

    MOCK(test2, ([](int i) -> int { return mock::CallOriginal<int, int>(i) + 1; }));
    g_assert(test2(2) == 5);

    MOCK_RETURN(test3, 1000);
    g_assert(test3(0) == 1000);

    TestClass a;

    {
        MOCK(&TestClass::F1, ([](TestClass *_, int i) -> int { return i + 1; }));
        g_assert(a.F1(10) == 11);
    }
    g_assert(a.F1(10) == 10);

    MOCK_RETURN(&TestClass::F2, true);
    g_assert(a.F2(0));

#if !defined(_WIN32)
    MOCK_VIRTUAL(&TestClass::V1, &a, [](TestClass *, int i) -> bool { return false; });
    g_assert(!a.V1(100));
#endif

    // mock virtual function TestClass::V2
    MOCK((mock::getVirtualFunctionByIndex<int (*)(TestClass *, int)>(&a, 2)), ([](TestClass *c, int i) -> int {
             if (i == 0) {
                 return 0;
             };
             return mock::CallOriginal<int, TestClass *, int>(c, i);
         }));

    g_assert(a.V2(0) == 0);
    g_assert(a.V2(100) == 101);

    std::random_device rd;
    std::default_random_engine e(rd());
    MOCK_RETURN(&std::default_random_engine::operator(), 8u);
    for (int i = 0; i < 20; ++i) {
        g_assert(e() == 8u);
    }

    return 0;
}