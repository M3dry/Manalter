#include "julia.h"
#include <julip/julip.hpp>

#include <print>

struct A {
    int x;
    float y;
    std::size_t l;

    JULIA_NAME(Test.A);
};

int main() {
    julip::init();

    auto a = A{
        .x = 100,
        .y = 0.0f,
        .l = 69
    };

    auto MainTest = julip::Main.create_sub_module("Test");
    MainTest.eval("struct A\nx::Int32\ny::Float32\nl::UInt\nend");
    julip::Function f = julip::Main.eval("(a::Test.A) -> println(a)");
    f(a);

    julip::destruct();
    return 0;
}
