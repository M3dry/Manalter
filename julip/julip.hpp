#pragma once

#include <concepts>
#include <cstring>
#include <functional>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <print>

#include <julia/julia.h>

namespace julip {
    namespace __impl {
        template <typename _Get_TypeName> consteval std::string_view getTypeName() {
            std::string_view name;

            if (name.empty()) {
                constexpr const char* beginStr = "_Get_TypeName =";
                constexpr size_t beginStrLen = 15; // Yes, I know...
                                                   // But isn't it better than strlen()?

                size_t begin, length;
                name = __PRETTY_FUNCTION__;

                begin = name.find(beginStr) + beginStrLen + 1;
                length = name.find("]", begin) - begin;
                name = name.substr(begin, length);
            }

            return name;
        }
    }

    void init();
    void destruct();

    uint64_t preserve(jl_value_t*);
    void release(uint64_t id);

#define gc_pause                                                                                                       \
    bool __gc_state_before__ = jl_gc_is_enabled();                                                                     \
    jl_gc_enable(false)
#define gc_unpause                                                                                                     \
    jl_gc_enable(__gc_state_before__);                                                                                 \
    jl_gc_safepoint()

    class value;

    namespace types {
#define __JULIAPODNAME __juliapodname

        template <typename T>
        concept POD = std::is_standard_layout_v<T> && (!std::is_enum_v<T>);

        template <typename T> struct TypeInfo;

        template <typename T> struct TypeName;
#define JULIA_STRUCT_NAME(T, name)                                                                                     \
    static_assert(julip::types::POD<T>);                                                                               \
    template <> struct julip::types::TypeName<T> {                                                                     \
        static constexpr const char* __JULIAPODNAME = #name;                                                           \
    };
#define JULIA_ENUM_NAME(T, name)                                                                                       \
    static_assert(std::is_enum_v<T>);                                                                                  \
    template <> struct julip::types::TypeName<T> {                                                                     \
        static constexpr const char* __JULIAPODNAME = #name;                                                           \
    };

        template <typename T>
        concept Typed = requires() {
            { TypeInfo<T>::type() } -> std::same_as<jl_value_t*>;
        };

        template <typename P>
        concept JuliaPOD = POD<P> && requires() {
            { TypeName<P>::__JULIAPODNAME } -> std::convertible_to<const char*>;
        };

        template <typename E>
        concept JuliaEnum = std::is_enum_v<E> && requires() {
            { TypeName<E>::__JULIAPODNAME } -> std::convertible_to<const char*>;
        };

        template <typename T>
        concept InPlace = std::is_same_v<typename TypeInfo<T>::inplace, std::true_type> && Typed<T>;

        template <typename T>
        concept _ToValue = requires(T t) {
            { TypeInfo<T>::to_value(t) } -> std::same_as<jl_value_t*>;
        };

        template <typename T>
        concept FromValue = requires(jl_value_t* v, bool check) {
            { TypeInfo<T>::from_value(v, check) } -> std::same_as<T>;
        };

        template <typename T>
        concept ToValue = _ToValue<std::remove_reference_t<T>> || _ToValue<std::remove_reference_t<T>&>;

        template <ToValue T>
        jl_value_t* to_value(T&& t) {
            if constexpr (_ToValue<std::remove_reference_t<T>>) {
                return TypeInfo<std::remove_reference_t<T>>::to_value(std::forward<T>(t));
            } else {
                return TypeInfo<std::remove_reference_t<T>&>::to_value(std::forward<T&>(t));
            }
        }
    }

    struct exception : public std::exception {
        exception(jl_value_t* e, const std::string& stacktrace);
        ~exception();

        const char* what() const noexcept final {
            return msg.c_str();
        };

        value* v;
        std::string msg;
    };

    struct bad_type : public std::exception {
        template <typename T>
        bad_type(T expected_type, const std::string& got)
            : expected_type("[JULIP BAD TYPE]: Expected `" + std::string(expected_type) + "`, got `" + got + "`") {
        }

        const char* what() const noexcept final {
            return expected_type.c_str();
        }

        std::string expected_type;
    };

    struct symbol {
        symbol(jl_sym_t* sym) : sym(sym) {
        }
        symbol(const char* str) : sym(jl_symbol(str)) {
        }
        symbol(const std::string& str) : sym(jl_symbol(str.c_str())) {
        }

        inline operator jl_sym_t*() {
            return sym;
        }
        inline operator jl_value_t*() {
            return (jl_value_t*)sym;
        }

        jl_sym_t* sym;
    };

    namespace __impl {
        template <typename... Args> jl_value_t* safe_call(jl_function_t* f, Args... args) {
            static jl_function_t* safe_call_f = jl_get_function(jl_main_module, "__julip__safe_call");
            static jl_value_t* exception_type = jl_get_global(jl_main_module, symbol("__julip__ExceptionInfo"));

            std::array<jl_value_t*, sizeof...(Args) + 1> args_v = {f, args...};

            gc_pause;
            jl_value_t* res = jl_call(safe_call_f, args_v.data(), args_v.size());
            auto id = preserve(res);

            if (jl_types_equal(jl_typeof(res), exception_type)) {
                jl_value_t* jl_e = jl_get_nth_field(res, 0);
                const char* stacktrace = jl_string_ptr(jl_get_nth_field(res, 1));

                auto e = exception(jl_e, stacktrace);

                release(id);
                gc_unpause;
                throw e;
            }

            release(id);
            gc_unpause;
            return res;
        }
    }

    class value {
      public:
        value(jl_value_t* v)
            : id(new uint64_t(julip::preserve(v)),
                 [](uint64_t* id) {
                     julip::release(*id);
                     delete id;
                 }),
              val(v) {
        }
        value(jl_value_t* v, uint64_t id)
            : id(new uint64_t(id),
                 [](uint64_t* id) {
                     julip::release(*id);
                     delete id;
                 }),
              val(v) {
        }
        value(const value& v) : id(v.id), val(v.val) {
        }
        template <typename T>
        requires (!std::is_same_v<value, std::remove_cvref_t<T>>)
        value(T&& t) {
            if constexpr (types::ToValue<T>) {
                gc_pause;
                val = types::to_value<T>(std::forward<T>(t));
                auto _id = julip::preserve(val);

                id = std::shared_ptr<uint64_t>(new uint64_t(_id), [](uint64_t* id) {
                    julip::release(*id);
                    delete id;
                });

                gc_unpause;
            } else {
                static_assert(false);
            }
        }


        template <typename T>
            requires types::FromValue<T>
        operator T() {
            return types::TypeInfo<T>::from_value(val);
        }

        template <types::ToValue... Args> value operator()(Args&&... args) const {
            return __impl::safe_call(val, types::to_value<Args>(std::forward<Args>(args))...);
        }

        value operator[](const std::string& field);

        inline jl_value_t* operator*() {
            return val;
        }

      private:
        std::shared_ptr<uint64_t> id;
        jl_value_t* val;
    };

    namespace __impl {
        struct FunctionWrapper {
            virtual ~FunctionWrapper() = default;
            virtual jl_value_t* operator()(jl_value_t** args, std::size_t nargs) = 0;
        };

        template <typename R, typename... Args>
            requires(types::ToValue<R> || std::is_same_v<R, void>) && (types::FromValue<Args> && ...)
        struct FunctionHolder : FunctionWrapper {
            std::function<R(Args...)> func;

            FunctionHolder(std::function<R(Args...)> f) : func(std::move(f)) {
            }

            jl_value_t* operator()(jl_value_t** jl_args, std::size_t nargs) override {
                assert(nargs == sizeof...(Args));

                return call_impl(jl_args, std::index_sequence_for<Args...>{});
            }

            template <std::size_t... Is> jl_value_t* call_impl(jl_value_t** jl_args, std::index_sequence<Is...>) {
                if constexpr (std::is_same_v<R, void>) {
                    func(types::TypeInfo<Args>::from_value(jl_args[Is])...);
                    return (jl_value_t*)jl_void_type;
                } else {
                    return types::to_value<R>(std::forward<R>(func(types::TypeInfo<Args>::from_value(jl_args[Is])...)));
                }
            }
        };

        template <typename T> struct function_traits;

        // For function pointer
        template <typename R, typename... Args> struct function_traits<R (*)(Args...)> {
            using signature = R(Args...);
        };

        // For std::function
        template <typename R, typename... Args> struct function_traits<std::function<R(Args...)>> {
            using signature = R(Args...);
        };

        // For lambdas and functors
        template <typename T> struct function_traits : function_traits<decltype(&T::operator())> {};

        // For lambda operator()
        template <typename C, typename R, typename... Args> struct function_traits<R (C::*)(Args...) const> {
            using signature = R(Args...);
        };

        extern std::unordered_map<uint64_t, std::unique_ptr<FunctionWrapper>> functions;
        extern uint64_t counter;
    }

    struct module {
        module(jl_module_t* mod) :mod(mod) {
        }

        inline operator jl_module_t*() {
            return mod;
        }
        inline operator jl_value_t*() {
            return (jl_value_t*)mod;
        }

        inline value operator[](const std::string& str) {
            return jl_get_global(mod, symbol(str));
        };

        inline void set_global(symbol sym, value v) {
            jl_set_global(mod, sym, *v);
        }

        template <types::ToValue T> inline void set_const(symbol sym, T&& t) {
            jl_set_const(mod, sym, types::to_value(std::forward<T>(t)));
        }

        template <typename F> uint64_t set_function(const std::string& name, F&& f) {
            using Signature = typename __impl::function_traits<std::decay_t<F>>::signature;

            return set_function_impl(name, std::function<Signature>(std::forward<F>(f)));
        }

        template <typename R, typename... Args>
            requires(types::Typed<R> || std::is_same_v<R, void>) && (types::Typed<Args> && ...)
        uint64_t set_function_impl(const std::string& name, std::function<R(Args...)> f) {
            __impl::functions[__impl::counter] = std::make_unique<__impl::FunctionHolder<R, Args...>>(std::move(f));

            std::stringstream f_str;
            uint64_t i = 1;
            f_str << "function " << name << "(";
            ((f_str << "__x__var" << i++ << "::" << jl_typename_str(types::TypeInfo<Args>::type()) << ","), ...);
            f_str << ")";
            if constexpr (!std::is_same_v<R, void>) {
                f_str << "::" << jl_typename_str(types::TypeInfo<R>::type());
            }
            f_str << "\n";
            f_str << "handle::UInt = UInt(" << __impl::counter << ")" << "\n";

            f_str << "args = Any[";
            i = 1;
            ((f_str << "__x__var" << i++ << "::" << jl_typename_str(types::TypeInfo<Args>::type()) << ","), ...);
            f_str << "]\n";

            if constexpr (!std::is_same_v<R, void>) {
                f_str << "return (";
            }
            f_str << "@ccall julip_ffi_call(handle::Csize_t, args::Ptr{Any}, length(args)::Csize_t)::";
            if constexpr (std::is_same_v<R, void>) {
                f_str << "Cvoid\n";
            } else {
                f_str << "Any)::" << jl_typename_str(types::TypeInfo<R>::type()) << "\n";
            }

            f_str << "end";

            std::string f_str_ = f_str.str();

            eval(f_str_);

            return __impl::counter++;
        }

        value eval(const std::string& jl);
        module create_sub_module(const std::string& name);
        value include(const std::string& path);

        jl_module_t* mod;
    };

    extern module Main;
    extern module Base;

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

    namespace __impl {
        std::pair<std::size_t, jl_datatype_t*> get_tuple_type(jl_value_t*);
        std::pair<jl_datatype_t*, std::size_t> get_array_type(jl_value_t*);
    }

    namespace types {
        template <> struct TypeInfo<value> {
            static jl_value_t* type() {
                return (jl_value_t*)jl_any_type;
            }

            static jl_value_t* to_vaue(value v)  {
                return *v;
            }

            static value from_value(jl_value_t* v, bool check = false) {
                return value(v);
            }
        };

        template <> struct TypeInfo<jl_value_t*> {
            static jl_value_t* to_value(jl_value_t* v) {
                return v;
            }

            static jl_value_t* from_value(jl_value_t* v, [[maybe_unused]] bool check = true) {
                return v;
            }
        };

#define PRIMITIVE_TYPEINFO(T, jl_type, box, _check, unbox)                                                             \
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
        static T from_value(jl_value_t* v, bool check = true) {                                                        \
            if (check && !_check) {                                                                                    \
                throw bad_type(__impl::getTypeName<T>(), jl_typeof_str(v));                                            \
            }                                                                                                          \
                                                                                                                       \
            return unbox(v);                                                                                           \
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

        template <POD P> struct TypeInfo<P&> {
            using inplace = std::true_type;

            static jl_value_t* type() {
                static jl_value_t* t = nullptr;
                if (t == nullptr) {
                    if constexpr (JuliaPOD<P>) {
                        t = jl_eval_string(TypeName<P>::__JULIAPODNAME);
                    } else {
                        t = jl_apply_type2(jl_get_global(jl_base_module, jl_symbol("NTuple")), jl_box_int64(sizeof(P)),
                                           (jl_value_t*)jl_uint8_type);
                    }
                }

                return t;
            }

            static jl_value_t* to_value(P& x) {
                auto* jl_p = jl_gc_allocobj(sizeof(P));

                jl_set_typeof(jl_p, type());
                std::memcpy(jl_value_ptr(jl_p), &x, sizeof(P));

                return jl_p;
            }
        };

        template <POD P> struct TypeInfo<P> {
            using inplace = std::true_type;

            static jl_value_t* type() {
                return TypeInfo<P&>::type();
            }

            static jl_value_t* to_value(P x) {
                return TypeInfo<P&>::to_value(x);
            }

            static P from_value(jl_value_t* v, bool check = true) {
                if (check && !jl_types_equal(jl_typeof(v), type())) {
                    throw bad_type(jl_typename_str(type()), jl_typeof_str(v));
                }

                return *(P*)jl_value_ptr(v);
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

            static FatPtr<T> from_value(jl_value_t* v, bool check = true) {
                T* data;
                std::size_t size;
                if (jl_is_array(v)) {
                    auto array_info = __impl::get_array_type(v);
                    if (check && !jl_types_equal(TypeInfo<T>::type(), (jl_value_t*)array_info.first) &&
                        array_info.second == 1) {
                        data = nullptr;
                        size = 0;
                    }

                    data = jl_array_data(v, T);
                    size = jl_array_len(v);
                } else if (jl_is_tuple(v)) {
                    auto tuple_info = __impl::get_tuple_type(v);

                    // FIXME: this only checks the first tuple element's type
                    // can't check that the type is NTUple{N, T} in constant time, maybe if I call into julia, but idk
                    if (check && !jl_types_equal(TypeInfo<T>::type(), (jl_value_t*)tuple_info.second)) {
                        ;
                        data = nullptr;
                        size = 0;
                    }

                    size = tuple_info.first;
                    data = (T*)jl_value_ptr(v);
                }
                if (check && data == nullptr && size == 0) {
                    throw bad_type("Vector{" + getTypeName<T>() + "} or NTuple{N, " + getTypeName<T>() + "}",
                                   jl_typeof_str(v));
                }

                if constexpr (COPY) {
                    T* data_cpy = (T*)(new uint_t[size * sizeof(T)]);
                    std::memcpy(data_cpy, data, sizeof(T) * size);

                    return {data_cpy, size};
                } else {
                    return {data, size};
                }
            };
        };

        template <InPlace T, std::size_t N> struct TypeInfo<std::array<T, N>> {
            using inplace = std::true_type;

            static jl_value_t* type() {
                static jl_value_t* ntuple = jl_get_global(jl_base_module, jl_symbol("NTuple"));
                static jl_value_t* t = jl_apply_type2(ntuple, jl_box_int64(N), TypeInfo<T>::type());

                return t;
            }

            static jl_value_t* to_value(std::array<T, N>& x) {
                static jl_value_t* t = type();

                auto* arr = jl_gc_allocobj(sizeof(T) * N);

                jl_set_typeof(arr, t);
                std::memcpy(jl_value_ptr(arr), x.data(), sizeof(T) * N);

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

            static std::string from_value(jl_value_t* v, bool check = true) {
                if (check && !jl_is_string(v)) {
                    throw bad_type("String", jl_typeof_str(v));
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

            static std::string_view from_value(jl_value_t* v, bool check = true) {
                if (check && !jl_is_string(v)) {
                    throw bad_type("String", jl_typeof_str(v));
                }

                char* data = jl_string_data(v);
                std::size_t size = jl_string_len(v);

                return std::string_view(data, size);
            }
        };

        template <Typed T> struct TypeInfo<T*> {
            using inplace = std::true_type;

            static jl_value_t* type() {
                static jl_value_t* t =
                    jl_apply_type1(jl_get_global(jl_base_module, jl_symbol("Ptr")), TypeInfo<T>::type());

                return t;
            }

            static jl_value_t* to_value(T* ptr) {
                auto* val = jl_box_uint64(reinterpret_cast<std::uintptr_t>(ptr));
                jl_set_typeof(val, type());

                return val;
            }

            static T* from_value(jl_value_t* v, bool check = true) {
                if (check && !jl_types_equal(type(), jl_typeof(v))) {
                    throw bad_type(jl_typename_str(type()), jl_typeof_str(v));
                }

                return (T*)jl_unbox_voidpointer(v);
            }
        };

        template <>
        struct TypeInfo<void*> {
            using inplace = std::true_type;

            static jl_value_t* type() {
                return (jl_value_t*)jl_voidpointer_type;
            }

            static jl_value_t* to_value(void* ptr) {
                return jl_box_voidpointer(ptr);
            }

            static void* from_value(jl_value_t* v, bool check = true) {
                if (check && !jl_types_equal(type(), jl_typeof(v))) {
                    throw bad_type(jl_typename_str(type()), jl_typeof_str(v));
                }

                return jl_unbox_voidpointer(v);
            }
        };

        template <Typed T>
            requires(sizeof(std::unique_ptr<T>) == sizeof(T*))
        struct TypeInfo<std::unique_ptr<T>&> {
            using inplace = std::true_type;

            static jl_value_t* type() {
                return TypeInfo<T*>::type();
            }

            static jl_value_t* to_value(std::unique_ptr<T>& ptr) {
                return TypeInfo<T*>::to_value(ptr.get());
            }
        };

        template <typename E>
            requires std::is_enum_v<E> // && ToValue<typename std::underlying_type<E>::type> && FromValue<typename
                                       // std::underlying_type<E>::type>
        struct TypeInfo<E> {
            using inplace = std::enable_if<InPlace<typename std::underlying_type<E>::type>, std::true_type>;
            using underlying = typename std::underlying_type<E>::type;

            static jl_value_t* type()
                requires Typed<underlying> or JuliaEnum<E>
            {
                if constexpr (JuliaEnum<E>) {
                    static jl_value_t* t = jl_eval_string(TypeName<E>::__JULIAPODNAME);

                    return t;
                } else {
                    return TypeInfo<underlying>::type();
                }
            }

            static jl_value_t* to_value(E e)
                requires _ToValue<underlying>
            {
                static jl_value_t* t = type();

                jl_value_t* v = TypeInfo<underlying>::to_value(static_cast<underlying>(e));
                jl_set_typeof(v, t);
                return v;
            }

            static E from_value(jl_value_t* v, bool check = true)
                requires FromValue<underlying>
            {
                static jl_value_t* t = type();
                if (check && !jl_types_equal(jl_typeof(v), t)) {
                    throw bad_type(jl_typename_str(t), jl_typeof_str(v));
                }

                return static_cast<E>(TypeInfo<underlying>::from_value(v, false));
            }
        };
    }
}

extern "C" __attribute__((visibility("default"))) jl_value_t* julip_ffi_call(size_t id, jl_value_t** args,
                                                                             size_t nargs);
