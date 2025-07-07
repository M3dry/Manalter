#pragma once

#include <concepts>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <julia/julia.h>

namespace julip {
    struct Function;

    namespace types {
#define __JULIAPODNAME __juliapodname
#define JULIA_NAME(name) static constexpr const char* __JULIAPODNAME = #name

        template <typename T>
        concept POD = std::is_trivial_v<T> && std::is_constructible_v<T>;

        template <typename T> struct TypeInfo;

        template <typename T>
        concept Typed = requires() {
            { TypeInfo<T>::type() } -> std::same_as<jl_value_t*>;
        };

        template <typename T>
        concept JuliaPOD = POD<T> && requires() {
            { T::__JULIAPODNAME } -> std::convertible_to<const char*>;
        };

        template <typename T>
        concept InPlace = std::is_same_v<typename TypeInfo<T>::inplace, std::true_type> && Typed<T>;

        template <typename T>
        concept ToValue = requires(T t) {
            { TypeInfo<T>::to_value(t) } -> std::same_as<jl_value_t*>;
        };

        template <typename Via, typename T>
        concept FromValue = requires(jl_value_t* v) {
            { TypeInfo<Via>::from_value(v) } -> std::same_as<T>;
        };
    }

    struct Value {
        Value(jl_value_t* val) : value(val) {
        }
        Value(jl_array_t* val) : value((jl_value_t*)val) {
        }
        Value(jl_datatype_t* val) : value((jl_value_t*)val) {
        }
        Value(jl_module_t* val) : value((jl_value_t*)val) {
        }
        Value(jl_sym_t* val) : value((jl_value_t*)val) {
        }

        inline operator jl_value_t*() {
            return value;
        }
        operator Function();

        template <typename T, typename Via = T>
            requires types::FromValue<Via, T>
        operator T() {
            return types::TypeInfo<Via>::from_value(value);
        }

        jl_value_t* value;
    };

    struct Symbol {
        Symbol(jl_sym_t* sym) : symbol(sym) {
        }
        Symbol(const char* str) : symbol(jl_symbol(str)) {
        }

        inline operator Value() {
            return (jl_value_t*)symbol;
        }
        inline operator jl_sym_t*() {
            return symbol;
        }
        inline operator jl_value_t*() {
            return (jl_value_t*)symbol;
        }

        jl_sym_t* symbol;
    };

    struct Module {
        Module(jl_module_t* mod) : module(mod) {
        }

        inline operator Value() {
            return (jl_value_t*)module;
        }
        inline operator jl_module_t*() {
            return module;
        }
        inline operator jl_value_t*() {
            return (jl_value_t*)module;
        }

        inline Value operator[](Symbol sym) {
            return jl_get_global(module, sym);
        };

        Value eval(const char* jl) {
            static jl_function_t* eval_f = jl_get_global(jl_base_module, jl_symbol("eval"));

            auto* quoted = jl_eval_string((std::string("quote ") + jl + " end").c_str());
            return quoted == nullptr ? nullptr : jl_call2(eval_f, *this, quoted);
        };

        inline Module create_sub_module(const char* name) {
            Symbol sym{name};
            Module mod = jl_new_module(sym, module);

            jl_set_const(module, sym, mod);

            return mod;
        }

        jl_module_t* module;
    };

    extern Module Main;
    extern Module Base;

    struct Function {
        Function(jl_function_t* f) : function(f) {};

        operator jl_function_t*() {
            return function;
        }

        template <types::ToValue... Args> Value operator()(Args&&... args) {
            std::array<jl_value_t*, sizeof...(Args)> args_v;

            std::size_t nargs = 0;
            auto set = [&](auto arg) { args_v[nargs++] = types::TypeInfo<decltype(arg)>::to_value(arg); };
            (set(args), ...);

            return jl_call(function, args_v.data(), nargs);
        }

        jl_function_t* function;
    };

    void init();
    void destruct();

    template <typename T, bool COPY = false> struct FatPtr {
        T* data;
        std::size_t size;

        FatPtr(T* data, std::size_t size) : data(data), size(size) {
        }
        FatPtr(std::vector<T>& vec) : data(vec.data()), size(vec.size()) {
        }
        FatPtr(std::span<T>& span) : data(span.data()), size(span.size()) {
        }
    };

    namespace util {
        std::pair<std::size_t, jl_datatype_t*> get_tuple_type(jl_value_t*);
        std::pair<jl_datatype_t*, std::size_t> get_array_type(jl_value_t*);
    }

    namespace types {
        template <> struct TypeInfo<jl_value_t*> {
            static jl_value_t* to_value(jl_value_t* v) {
                return v;
            }
        };

        template <> struct TypeInfo<jl_array_t*> {
            static jl_value_t* to_value(jl_array_t* arr) {
                return (jl_value_t*)arr;
            }
        };

        template <> struct TypeInfo<jl_datatype_t*> {
            static jl_value_t* to_value(jl_datatype_t* dt) {
                return (jl_value_t*)dt;
            }
        };

        template <> struct TypeInfo<jl_module_t*> {
            static jl_value_t* to_value(jl_module_t* mod) {
                return (jl_value_t*)mod;
            }
        };

        template <> struct TypeInfo<jl_sym_t*> {
            static jl_value_t* to_value(jl_sym_t* sym) {
                return (jl_value_t*)sym;
            }
        };

        template <> struct TypeInfo<Value> {
            static jl_value_t* to_value(Value v) {
                return v;
            }
        };

        template <> struct TypeInfo<Symbol> {
            static jl_value_t* to_value(Symbol sym) {
                return sym;
            }
        };

        template <> struct TypeInfo<Module> {
            static jl_value_t* to_value(Module mod) {
                return mod;
            }
        };

        template <> struct TypeInfo<Function> {
            static jl_value_t* to_value(Function f) {
                return f;
            }
        };

#define PRIMITIVE_TYPEINFO(T, jl_type, box, check, unbox)                                                              \
    template <> struct TypeInfo<T> {                                                                                   \
        using inplace = std::true_type;                                                                                \
                                                                                                                       \
        static jl_value_t* type() {                                                                                    \
            return (jl_value_t*)jl_type;                                                                               \
        }                                                                                                              \
                                                                                                                       \
        static jl_value_t* to_value(T x) {                                                                             \
            return box(x);                                                                                             \
        }                                                                                                              \
                                                                                                                       \
        static T from_value(jl_value_t* v) {                                                                           \
            if (!check) {                                                                                              \
                assert(false && "TODO: add exceptions");                                                               \
            }                                                                                                          \
                                                                                                                       \
            return unbox(v);                                                                                           \
        }                                                                                                              \
                                                                                                                       \
        static T* ref_from_value(jl_value_t* v) {                                                                      \
            return (T*)jl_data_ptr(v);                                                                                 \
        }                                                                                                              \
    };

        PRIMITIVE_TYPEINFO(uint64_t, jl_uint64_type, jl_box_uint64, jl_is_uint64(v), jl_unbox_uint64)
        PRIMITIVE_TYPEINFO(uint32_t, jl_uint32_type, jl_box_uint32, jl_is_uint32(v), jl_unbox_uint32)
        PRIMITIVE_TYPEINFO(uint16_t, jl_uint16_type, jl_box_uint16, jl_is_uint16(v), jl_unbox_uint16)
        PRIMITIVE_TYPEINFO(uint8_t, jl_uint8_type, jl_box_uint8, jl_is_uint8(v), jl_unbox_uint8)
        PRIMITIVE_TYPEINFO(int64_t, jl_int64_type, jl_box_int64, jl_is_int64(v), jl_unbox_int64)
        PRIMITIVE_TYPEINFO(int32_t, jl_int32_type, jl_box_int32, jl_is_int32(v), jl_unbox_int32)
        PRIMITIVE_TYPEINFO(int16_t, jl_int16_type, jl_box_int16, jl_is_int16(v), jl_unbox_int16)
        PRIMITIVE_TYPEINFO(int8_t, jl_int8_type, jl_box_int8, jl_is_int8(v), jl_unbox_int8)
        PRIMITIVE_TYPEINFO(double, jl_float64_type, jl_box_float64, jl_typeis(v, jl_float64_type), jl_unbox_float64)
        PRIMITIVE_TYPEINFO(float, jl_float32_type, jl_box_float32, jl_typeis(v, jl_float32_type), jl_unbox_float32)
        PRIMITIVE_TYPEINFO(bool, jl_bool_type, jl_box_bool, jl_is_bool(v), jl_unbox_bool)

        template <POD P> struct TypeInfo<P> {
            using inplace = std::true_type;

            static jl_value_t* type() {
                static jl_value_t* t = nullptr;
                if (t == nullptr) {
                    if constexpr (JuliaPOD<P>) {
                        t = Main.eval(P::__JULIAPODNAME);
                    } else {
                        t = jl_apply_type2(Base["NTuple"], jl_box_int64(sizeof(P)), (jl_value_t*)jl_uint8_type);
                    }
                }

                return t;
            }

            static jl_value_t* to_value(P& x) {
                auto* jl_p = jl_gc_allocobj(sizeof(P));

                jl_set_typeof(jl_p, type());
                std::memcpy(jl_data_ptr(jl_p), &x, sizeof(P));

                return jl_p;
            }

            static P from_value(jl_value_t* v) {
                if (!jl_types_equal(v, TypeInfo<P>::type())) {
                    assert(false && "TODO: add exceptions");
                }

                return *(P*)jl_data_ptr(v);
            }

            static P* ref_from_value(jl_value_t* v) {
                if (!jl_types_equal(v, TypeInfo<P>::type())) {
                    assert(false && "TODO: add exceptions");
                }

                return (P*)jl_data_ptr(v);
            }
        };

        template <InPlace T, bool COPY> struct TypeInfo<FatPtr<T, COPY>> {
            static jl_value_t* type() {
                static jl_value_t* arr_type = jl_apply_array_type(TypeInfo<T>::type(), 1);

                return arr_type;
            }

            static jl_value_t* to_value(FatPtr<T> x) {
                if constexpr (COPY) {
                    jl_array_t* arr = jl_alloc_array_1d(type(), x.size);

                    std::memcpy(jl_array_data_(arr), x.data, sizeof(T) * x.size);

                    return (jl_value_t*)arr;
                } else {
                    return (jl_value_t*)jl_ptr_to_array_1d(type(), x.data, x.size, 0);
                }
            }

            static FatPtr<T> from_value(jl_value_t* v) {
                T* data;
                std::size_t size;
                if (jl_is_array(v)) {
                    auto array_info = julip::util::get_array_type(v);
                    if (!jl_types_equal(TypeInfo<T>::type(), (jl_value_t*)array_info.first) && array_info.second == 1) {
                        assert(false && "TODO: add exceptions");
                    }

                    data = jl_array_data(v, T);
                    size = jl_array_len(v);
                } else if (jl_is_tuple(v)) {
                    auto tuple_info = julip::util::get_tuple_type(v);

                    // FIXME: this only checks the first tuple element's type
                    // can't check that the type is NTUple{N, T} in constant time, maybe if I call into julia, but idk
                    if (!jl_types_equal(TypeInfo<T>::type(), (jl_value_t*)tuple_info.second)) {
                        ;
                        assert(false && "TODO: add exceptions");
                    }

                    size = tuple_info.first;
                    data = (T*)jl_data_ptr(v);
                }

                return {data, size};
            };
        };

        template <InPlace T, std::size_t N> struct TypeInfo<std::array<T, N>> {
            using inplace = std::true_type;

            static jl_value_t* type() {
                static jl_value_t* ntuple = Base["NTuple"];
                static jl_value_t* t = jl_apply_type2(ntuple, jl_box_int64(N), TypeInfo<T>::type());

                return t;
            }

            static jl_value_t* to_value(std::array<T, N>& x) {
                static jl_value_t* t = type();

                auto* arr = jl_gc_allocobj(sizeof(T) * N);

                jl_set_typeof(arr, t);
                std::memcpy(jl_data_ptr(arr), x.data(), sizeof(T) * N);

                return arr;
            }
        };

        // NOTE: std::tuple and std::optional can do anything when it comes to layout so womp womp
        // template <InPlace... Ts> struct TypeInfo<std::tuple<Ts...>> {
        //     using inplace = std::true_type;
        // };

        template <> struct TypeInfo<std::string> {
            static jl_value_t* type() {
                return (jl_value_t*)jl_string_type;
            }

            static jl_value_t* to_value(std::string& str) {
                return jl_pchar_to_string(str.data(), str.size());
            }

            static std::string from_value(jl_value_t* v) {
                if (!jl_is_string(v)) {
                    assert(false && "TODO: add exceptions");
                }

                char* data = jl_string_data(v);
                std::size_t size = jl_string_len(v);

                return std::string(data, size);
            }
        };

        template <> struct TypeInfo<std::string_view> {
            static jl_value_t* type() {
                return (jl_value_t*)jl_string_type;
            }

            static jl_value_t* to_value(std::string_view& str) {
                return jl_pchar_to_string(str.data(), str.size());
            }

            static std::string_view from_value(jl_value_t* v) {
                if (!jl_is_string(v)) {
                    assert(false && "TODO: add exceptions");
                }

                char* data = jl_string_data(v);
                std::size_t size = jl_string_len(v);

                return std::string_view(data, size);
            }
        };

        template <Typed T> struct TypeInfo<T*> {
            using inplace = std::true_type;

            static jl_value_t* type() {
                static jl_value_t* ptr = Base["Ptr"];
                static jl_value_t* t = jl_apply_type1(ptr, TypeInfo<T>::type());

                return t;
            }

            static jl_value_t* to_value(T* ptr) {
                auto* val = jl_box_uint64(reinterpret_cast<std::uintptr_t>(ptr));
                jl_set_typeof(val, type());

                return val;
            }
        };

        template <InPlace T> struct TypeInfo<T&> {
            static jl_value_t* to_value(T& x) {
                return TypeInfo<FatPtr<T>>::to_value({&x, 1});
            };
        };
    }
}
