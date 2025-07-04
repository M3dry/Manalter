#include "julip.hpp"

#include <julia.h>
#include <print>

JULIA_DEFINE_FAST_TLS

namespace julip {
    jl_value_t* __to_value(const char* str) {
        return jl_cstr_to_string(str);
    }

    jl_value_t* __to_value(uint64_t n) {
        return jl_box_uint64(n);
    }

    jl_value_t* __to_value(uint32_t n) {
        return jl_box_uint32(n);
    }

    jl_value_t* __to_value(uint16_t n) {
        return jl_box_uint16(n);
    }

    jl_value_t* __to_value(uint8_t n) {
        return jl_box_uint8(n);
    }

    jl_value_t* __to_value(int64_t n) {
        return jl_box_int64(n);
    }

    jl_value_t* __to_value(int32_t n) {
        return jl_box_int32(n);
    }

    jl_value_t* __to_value(int16_t n) {
        return jl_box_int16(n);
    }

    jl_value_t* __to_value(int8_t n) {
        return jl_box_int8(n);
    }

    jl_value_t* __to_value(float n) {
        return jl_box_float32(n);
    }

    jl_value_t* __to_value(double n) {
        return jl_box_float64(n);
    }

    jl_value_t* __to_value(bool b) {
        return jl_box_bool(b);
    }

    Value::operator Function() {
        if (!jl_isa(value, (jl_value_t*)jl_function_type)) {
            assert(false && "TODO: add exceptions");
        }

        return value;
    };

    state::state() {
        jl_init();
    }

    state::state(state&& s) noexcept : _valid(true) {
        s._valid = false;
    }

    state::~state() {
        if (_valid) jl_atexit_hook(0);
    }

    Value state::eval_str(const char* jl, Module mod) {
        static Function eval_f = (*this)["eval", jl_base_module];

        auto* quoted = jl_eval_string((std::string("begin ") + jl + "end").c_str());
        return eval_f(mod, quoted);
    }

    Value state::operator[](Symbol sym, Module mod) {
        return jl_get_global(mod, sym.symbol);
    }
}
