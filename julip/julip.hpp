#pragma once

#include <concepts>

#include "julia.h"

namespace julip {
    template <typename V>
    concept IsValue = requires(V value) {
        { static_cast<jl_value_t*>(value) };
    };

    jl_value_t* __to_value(const char* str);
    jl_value_t* __to_value(uint64_t n);
    jl_value_t* __to_value(uint32_t n);
    jl_value_t* __to_value(uint16_t n);
    jl_value_t* __to_value(uint8_t n);
    jl_value_t* __to_value(int64_t n);
    jl_value_t* __to_value(int32_t n);
    jl_value_t* __to_value(int16_t n);
    jl_value_t* __to_value(int8_t n);
    jl_value_t* __to_value(float n);
    jl_value_t* __to_value(double n);
    jl_value_t* __to_value(bool b);

    template <typename T>
    concept ToValue = requires(T t) {
        { __to_value(t) } -> std::same_as<jl_value_t*>;
    };

    template <IsValue V>
    jl_value_t* __to_value(V v) {
        return v;
    }

    struct Symbol {
        Symbol(const char* str) : symbol(jl_symbol(str)) {};
        Symbol(jl_sym_t* sym) : symbol(sym) {};

        operator jl_sym_t*() {
            return symbol;
        }

        jl_sym_t* symbol;
    };

    struct Module {
        Module(jl_module_t* mod) : module(mod) {};

        operator jl_module_t*() {
            return module;
        }

        jl_module_t* module;
    };

    struct Function;

    struct Value {
        Value(jl_value_t* value) : value(value) {};

        operator jl_value_t*() {
            return value;
        }

        operator Module() {
            if (!jl_is_module(value)) {
                assert(false && "TODO: add exceptions");
            }

            return (jl_module_t*)value;
        }

        operator Function();

        operator const char*() {
            if (!jl_is_string(value)) {
                assert(false && "TODO: add exceptions");
            }

            return jl_string_ptr(value);
        }

        operator uint64_t() {
            if (!jl_is_uint64(value)) {
                assert(false && "TODO: add exceptions");
            }

            return jl_unbox_uint64(value);
        }

        operator uint32_t() {
            if (!jl_is_uint32(value)) {
                assert(false && "TODO: add exceptions");
            }

            return jl_unbox_uint32(value);
        }

        operator uint16_t() {
            if (!jl_is_uint16(value)) {
                assert(false && "TODO: add exceptions");
            }

            return jl_unbox_uint16(value);
        }

        operator uint8_t() {
            if (!jl_is_uint8(value)) {
                assert(false && "TODO: add exceptions");
            }

            return jl_unbox_uint8(value);
        }

        operator int64_t() {
            if (!jl_is_int64(value)) {
                assert(false && "TODO: add exceptions");
            }

            return jl_unbox_int64(value);
        }

        operator int32_t() {
            if (!jl_is_int32(value)) {
                assert(false && "TODO: add exceptions");
            }

            return jl_unbox_int32(value);
        }

        operator int16_t() {
            if (!jl_is_int16(value)) {
                assert(false && "TODO: add exceptions");
            }

            return jl_unbox_int16(value);
        }

        operator int8_t() {
            if (!jl_is_int8(value)) {
                assert(false && "TODO: add exceptions");
            }

            return jl_unbox_int8(value);
        }

        operator float() {
            if (!jl_typeis(value, jl_float32_type)) {
                assert(false && "TODO: add exceptions");
            }

            return jl_unbox_float32(value);
        }

        operator double() {
            if (!jl_typeis(value, jl_float64_type)) {
                assert(false && "TODO: add exceptions");
            }

            return jl_unbox_float64(value);
        }

        operator bool() {
            if (!jl_is_bool(value)) {
                assert(false && "TODO: add exceptions");
            }

            return jl_unbox_bool(value);
        }

        jl_value_t* value;
    };

    struct Function {
        Function(jl_function_t* f) : function(f) {};

        template <ToValue... Args>
        Value operator()(Args... vals) {
            std::array<jl_value_t*, sizeof...(vals)> args;

            auto set = [&](std::size_t i, auto v) { args[i] = __to_value(v); };

            std::size_t i = 0;
            (set(i++, vals), ...);

            Value res = jl_call(function, args.data(), args.size());
            // TODO: excepetion handling
            return res;
        };

        operator jl_function_t*() {
            return function;
        }

        jl_function_t* function;
    };

    class state {
      public:
        state();
        state(state&&) noexcept;
        state& operator=(state&&) noexcept = default;
        ~state();

        Value eval_str(const char*, Module = jl_main_module);
        Value operator[](Symbol, Module = jl_main_module);

      private:
        bool _valid = true;
    };
}
