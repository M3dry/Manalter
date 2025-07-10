#include "julip.hpp"
#include "julia.h"
#include <cstdint>
#include <print>

JULIA_DEFINE_FAST_TLS

namespace julip {
    module Main = nullptr;
    module Base = nullptr;

    void init() {
        jl_init();

        Main = jl_main_module;
        Base = jl_base_module;

        jl_eval_string(R"(
if !isdefined(Main, :__julip_refs)
    const __julip__objs = Dict{UInt64, Base.RefValue{Any}}()
    const __julip__counter = Base.RefValue(UInt64(0));
    const __julip__lock = Base.ReentrantLock()

    function __julip__add_obj(ptr::UInt64)::UInt64
        lock(__julip__lock)

        id = __julip__counter[]
        __julip__counter[] += 1

        __julip__objs[id] = Ref{Any}(unsafe_pointer_to_objref(Ptr{Any}(ptr)))

        unlock(__julip__lock)
        return id;
    end

    function __julip__remove_obj(id::UInt64)
        lock(__julip__lock)
        delete!(__julip__objs, id)
        unlock(__julip__lock)
    end
end

if !isdefined(Main, :__julip__safe_call)
    struct __julip__ExceptionInfo
        e::Exception
        backtrace::String
    end

    function __julip__safe_call(f::Function, args...)
        try
            return f(args...)
        catch e
            return __julip__ExceptionInfo(e, sprint(Base.showerror, e, catch_backtrace()))
        end
    end
end
)");
    }

    void destruct() {
        jl_atexit_hook(0);
    }

    uint64_t preserve(jl_value_t* v) {
        bool gc_state = jl_gc_is_enabled();
        jl_gc_enable(false);

        static jl_function_t* add_obj = jl_get_function(jl_main_module, "__julip__add_obj");
        auto id = jl_unbox_uint64(jl_call1(add_obj, jl_box_uint64(reinterpret_cast<uint64_t>(v))));

        jl_gc_enable(gc_state);
        return id;
    }

    void release(uint64_t id) {
        bool gc_state = jl_gc_is_enabled();
        jl_gc_enable(false);

        static jl_function_t* remove_obj = jl_get_function(jl_main_module, "__julip__remove_obj");
        jl_call1(remove_obj, jl_box_uint64(id));

        jl_gc_enable(gc_state);
    }

    value value::operator[](const std::string& field) {
        if (jl_is_module(val)) {
            return jl_get_global((jl_module_t*)val, symbol(field));
        }
        if (jl_is_structtype(val)) {
            return jl_get_field(val, field.c_str());
        }

        return nullptr;
    }

    value module::eval(const std::string& jl) {
        static jl_function_t* eval_f = jl_get_function(jl_base_module, "eval");

        auto* quoted = jl_eval_string((std::string("quote ") + jl + " end").c_str());
        return quoted == nullptr ? nullptr : __impl::safe_call(eval_f, *this, quoted);
    };

    module module::create_sub_module(const std::string& name) {
        symbol sym{name};
        module m = jl_new_module(sym, mod);

        jl_set_const(*this, sym, m);

        return mod;
    }

    value module::include(const std::string& path) {
        static jl_function_t* include = jl_get_function(jl_base_module, "include");

        return jl_call2(include, *this, jl_pchar_to_string(path.data(), path.size()));
    }

    exception::exception(jl_value_t* e, const std::string& stacktrace)
        : v(new value(e)), msg("[JULIP EXCEPTION]: " + stacktrace) {
    }

    exception::~exception() {
        delete v;
    }
}

namespace julip::__impl {
    std::unordered_map<uint64_t, std::unique_ptr<FunctionWrapper>> functions{};
    uint64_t counter = 0;

    std::pair<std::size_t, jl_datatype_t*> get_tuple_type(jl_value_t* v) {
        if (!jl_is_tuple(v)) {
            throw bad_type("Tuple or NTuple", jl_typeof_str(v));
        }

        jl_value_t* v_type = jl_typeof(v);
        if (!jl_is_datatype(v_type)) {
            throw bad_type("DataType", jl_typeof_str(v_type));
        }

        jl_datatype_t* v_dt = (jl_datatype_t*)v_type;
        jl_svec_t* params = v_dt->parameters;
        jl_value_t* type = jl_svecref(params, 1);

        return {jl_svec_len(params), (jl_datatype_t*)type};
    }

    std::pair<jl_datatype_t*, std::size_t> get_array_type(jl_value_t* v) {
        if (!jl_is_array(v)) {
            throw bad_type("Array{T, N}", jl_typeof_str(v));
        }

        jl_value_t* v_type = jl_typeof(v);
        if (!jl_is_datatype(v_type)) {
            throw bad_type("DataType", jl_typeof_str(v));
        }

        return {(jl_datatype_t*)jl_tparam0(v_type), jl_unbox_int64(jl_tparam1(v_type))};
    }
}

extern "C" jl_value_t* julip_ffi_call(size_t id, jl_value_t** args, size_t nargs) {
    auto f = julip::__impl::functions[id].get();

    return f->operator()(args, nargs);
}
