#include <julip/julip.hpp>

struct A {
    int x;
    float y;
    std::size_t l;

    JULIA_STRUCT_NAME(A);
};

enum BCD {
    B = 0,
    C,
    D,
};
JULIA_ENUM_NAME(BCD, BCD)

int main(int argc, char** argv) {
    julip::init();

    julip::Main.eval("struct A\nx::Int32\ny::Float32\nl::UInt\nend");
    julip::Main.eval("@enum BCD::UInt32 B=0 C D");

    std::unique_ptr<int64_t> p = std::make_unique<int64_t>(100);
    auto a = A{
        .x = 100,
        .y = 0.0f,
        .l = 69,
    };

    auto f = julip::Main.eval(R"((a, b, c) -> begin
    println(a)
    println(b)
    println(c)
end)");

    while (true) {
        f(p, 100, a);
    }

    // julip::Main.set_function("f", [](int x, int y) -> int {
    //     std::println("hello");
    //     return x + y;
    // });
    //
    // julip::Main.eval("println(f(Int32(100), Int32(200)))");

    julip::destruct();
    return 0;
}
