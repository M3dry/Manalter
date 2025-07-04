#include "julia.h"
#include <julip/julip.hpp>

#include <print>

int main() {
    julip::state state;

    state.eval_str("xyz = 100");
    std::println("{}", (int64_t)state["xyz"]);

    return 0;
}
