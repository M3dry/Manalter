#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

template <typename... Ts> struct type_set {};
namespace typeset {
    template <typename... Ts> struct pack {
        std::tuple<Ts&&...> data;

        explicit pack(Ts&&... args) : data(std::forward<Ts>(args)...) {}
    };

    template <typename... Ts> auto make_pack(Ts&&... args) {
        return pack<Ts&&...>{std::forward<Ts>(args)...};
    }

    namespace __ {
        template <typename... Ts, typename F, std::size_t... Is>
        void with_pack_impl(pack<Ts...>& pack, F&& f, std::index_sequence<Is...>) {
            std::forward<F>(f)(std::forward<Ts>(std::get<Is>(pack.data))...);
        }
    }

    template <typename... Ts, typename F> void with_pack(pack<Ts...>&& pack, F&& f) {
        __::with_pack_impl(pack, std::forward<F>(f), std::index_sequence_for<Ts...>{});
    };

    template <typename T> struct is_pack {
        static constexpr bool value = false;
    };
    template <typename... Ts> struct is_pack<pack<Ts...>> {
        static constexpr bool value = true;
    };

    template <typename T> static constexpr bool is_pack_v = is_pack<T>::value;

    template <typename T, typename... Ts>
    concept in_set_v = (std::is_same_v<T, Ts> || ...);

    template <typename... Ts> struct unique;
    template <> struct unique<> {
        static constexpr bool value = true;
    };
    template <typename T, typename... Ts> struct unique<T, Ts...> {
        static constexpr bool value = !in_set_v<T, Ts...> && unique<Ts...>::value;
    };

    template <typename... Ts>
    concept unique_v = unique<Ts...>::value;

    template <template <typename> typename Pred, typename... Ts> struct find_first;
    template <template <typename> typename Pred, typename F, typename... Ts>
        requires Pred<F>::value
    struct find_first<Pred, F, Ts...> {
        static constexpr std::size_t value = 0;
    };
    template <template <typename> typename Pred, typename T, typename... Ts> struct find_first<Pred, T, Ts...> {
        static constexpr std::size_t value = 1 + find_first<Pred, Ts...>::value;
    };

    template <template <typename> typename Pred, typename... Ts>
    static constexpr std::size_t find_first_v = find_first<Pred, Ts...>::value;

    template <typename Fixed> struct is_same_to {
        template <typename U> struct apply : std::is_same<Fixed, U> {};
    };

    template <typename T> struct void_f {
        static constexpr void f() {}
    };

    template <std::size_t n, typename... Ts> struct nth;
    template <typename t, typename... Ts> struct nth<0, t, Ts...> {
        using T = t;
    };
    template <std::size_t n, typename t, typename... Ts>
        requires(sizeof...(Ts) + 1 > n)
    struct nth<n, t, Ts...> {
        using T = typename nth<n - 1, Ts...>::T;
    };

    template <std::size_t n, typename... Ts> using nth_t = typename nth<n, Ts...>::T;

    template <typename T, typename Y> struct is_subset;
    template <template <typename...> typename T, template <typename...> typename Y, unique_v... Ts, unique_v... Ys>
    struct is_subset<T<Ts...>, Y<Ys...>> {
        static constexpr bool value = (in_set_v<Ts, Ys...> && ...);
    };

    template <typename T, typename Y> static constexpr bool is_subset_v = is_subset<T, Y>::value;

    template <typename T, typename Y> struct is_same_set {
        static constexpr bool value = is_subset_v<T, Y> && is_subset_v<Y, T>;
    };

    template <typename T, typename Y> static constexpr bool is_same_set_v = is_same_set<T, Y>::value;

    template <typename... Ts> struct on_unique_unordered_pairs;
    template <> struct on_unique_unordered_pairs<> {
        template <typename F> static consteval bool apply(F&&) {
            return true;
        }
    };
    template <typename T> struct on_unique_unordered_pairs<T> {
        template <typename F> static consteval bool apply(F&&) {
            return true;
        }
    };
    template <typename T, typename U, typename... Rest> struct on_unique_unordered_pairs<T, U, Rest...> {
        template <typename F> static consteval bool apply(F&& f) {
            bool res = true;

            res = res && f.template operator()<T, U>();

            ((res = res && (f.template operator()<T, Rest>())), ...);

            res = res && on_unique_unordered_pairs<U, Rest...>::apply(std::forward<F>(f));

            return res;
        }
    };

    namespace __ {
        struct checker {
            template <typename T, typename U> consteval bool operator()() const {
                return !is_same_set_v<T, U>;
            }
        };
    }

    template <typename... Sets> struct different_sets {
        static constexpr bool value = on_unique_unordered_pairs<Sets...>::apply(__::checker{});
    };

    template <typename... Sets> static constexpr bool different_sets_v = different_sets<Sets...>::value;

    template <template <typename, typename> typename F, typename T> struct curry2 {
        template <typename U> struct apply : F<T, U> {};
    };

    template <typename Set> struct to_unique;
    template <template <typename...> typename Set, typename... Ts> struct to_unique<Set<Ts...>> {
        template <typename...> struct __impl;

        template <typename... Res> struct __impl<Set<>, Set<Res...>> {
            using T = Set<Res...>;
        };

        template <typename U, typename... Rest, typename... Res> struct __impl<Set<U, Rest...>, Set<Res...>> {
            using T = std::conditional_t<in_set_v<U, Res...>, __impl<Set<Rest...>, Set<Res...>>,
                                         __impl<Set<Rest...>, Set<Res..., U>>>::T;
        };

        using T = __impl<Set<Ts...>, Set<>>::T;
    };

    template <typename T> using to_unique_t = to_unique<T>::T;

    template <typename T, typename Y, template <typename...> typename Set = type_set> struct set_union;
    template <template <typename...> typename U, template <typename...> typename Y, typename... Us, typename... Ys,
              template <typename...> typename Set>
    struct set_union<U<Us...>, Y<Ys...>, Set> {
        using T = to_unique_t<Set<Us..., Ys...>>;
    };

    template <typename U, typename Y, template <typename...> typename Set = type_set>
    using set_union_t = set_union<U, Y, Set>::T;

    template <typename T, typename Y, template <typename...> typename Set = type_set> struct set_difference;
    template <template <typename...> typename U, typename Y, template <typename...> typename Set>
    struct set_difference<U<>, Y, Set> {
        using T = Set<>;
    };
    template <template <typename...> typename U, typename A, typename... Us, template <typename...> typename Y,
              typename... Ys, template <typename...> typename Set>
    struct set_difference<U<A, Us...>, Y<Ys...>, Set> {
        using T =
            std::conditional_t<in_set_v<A, Ys...>, typename set_difference<U<Us...>, Y<Ys...>, Set>::T,
                               set_union_t<type_set<A>, typename set_difference<U<Us...>, Y<Ys...>, Set>::T, Set>>;
    };

    template <typename T, typename Y, template <typename...> typename Set = type_set>
    using set_difference_t = set_difference<T, Y, Set>::T;

    template <typename T> struct set_size;
    template <template <typename...> typename T, typename... Ts> struct set_size<T<Ts...>> {
        static constexpr std::size_t value = sizeof...(Ts);
    };

    template <typename T>
    static constexpr std::size_t set_size_v = set_size<T>::value;

    template <typename T>
    struct wrapped {
        using type = T;
    };
}
