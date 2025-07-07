#include "julip.hpp"
#include "julia.h"
#include <cstdint>
#include <print>

JULIA_DEFINE_FAST_TLS

namespace julip {
    Module Main = nullptr;
    Module Base = nullptr;

    Value::operator Function() {
        return (jl_function_t*)value;
    }

    void init() {
        jl_init();

        Main = jl_main_module;
        Base = jl_base_module;
    }

    void destruct() {
        jl_atexit_hook(0);
    }
}

namespace julip::util {
    std::pair<std::size_t, jl_datatype_t*> get_tuple_type(jl_value_t* v) {
        jl_value_t* v_type = jl_typeof(v);
        if (!jl_is_datatype(v_type)) {
            assert(false && "datatype check; TODO: add exceptions");
        }

        jl_datatype_t* v_dt = (jl_datatype_t*)v_type;
        if (v_dt->name != jl_tuple_typename) {
            assert(false && "typename check; TODO: add exceptions");
        }

        jl_svec_t* params = v_dt->parameters;
        jl_value_t* type = jl_svecref(params, 1);

        return {jl_svec_len(params), (jl_datatype_t*)type};
    }

    std::pair<jl_datatype_t*, std::size_t> get_array_type(jl_value_t* v) {
        if (!jl_is_array(v)) {
            assert(false && "array check; TODO: add exceptions");
        }

        jl_value_t* v_type = jl_typeof(v);
        if (!jl_is_datatype(v_type)) {
            assert(false && "datatype check; TODO: add exceptions");
        }

        return {(jl_datatype_t*)jl_tparam0(v_type), jl_unbox_int64(jl_tparam1(v_type))};
    }
}
