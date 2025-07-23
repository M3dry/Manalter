#pragma once

#include <array>
#include <cassert>
#include <concepts>
#include <print>
#include <span>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <vector>

#include "typeset.hpp"

template <typeset::unique_v... Ts>
    requires((std::move_constructible<Ts> || std::copy_constructible<Ts>) && ...)
class multi_vector {
  public:
    multi_vector(std::size_t capacity = 5) : _capacity(std::max<std::size_t>(capacity, 1)), _size(0) {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((vectors[Is] = ::operator new(capacity * sizeof(typeset::nth_t<Is, Ts...>))), ...);
        }(std::index_sequence_for<Ts...>{});
    }

    multi_vector(const multi_vector& mv)
        requires(std::is_copy_constructible_v<Ts> && ...)
        : _capacity(mv._capacity), _size(mv._size) {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (
                [&](std::size_t _) {
                    using T = typeset::nth_t<Is, Ts...>;

                    vectors[Is] = ::operator new(_capacity * sizeof(T));

                    for (std::size_t i = 0; i < _size; i++) {
                        std::construct_at(&reinterpret_cast<T*>(vectors[Is])[i],
                                          reinterpret_cast<T*>(mv.vectors[Is])[i]);
                    }
                }(Is),
                ...);
        }(std::index_sequence_for<Ts...>{});
    }
    multi_vector& operator=(const multi_vector&) = default;

    multi_vector(multi_vector&& mv) noexcept
        : _capacity(mv._capacity), _size(mv._size), vectors(std::move(mv.vectors)) {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((mv.vectors[Is] = nullptr), ...);
        }(std::index_sequence_for<Ts...>{});
    };
    multi_vector& operator=(multi_vector&&) noexcept = default;

    ~multi_vector() {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ([&]<std::size_t I>(std::integral_constant<std::size_t, I>) {
                if (vectors[I] == nullptr) return;

                for (std::size_t i = 0; i < _size; i++) {
                    std::destroy_at(&reinterpret_cast<typeset::nth_t<Is, Ts...>*>(vectors[Is])[i]);
                }

                ::operator delete(vectors[Is]);
            }(std::integral_constant<std::size_t, Is>{}), ...);
        }(std::index_sequence_for<Ts...>{});
    }

    void reserve(std::size_t new_capacity) {
        if (new_capacity > _capacity) resize(new_capacity);
    }

    void shrink_to_fit() {
        resize(_size);
    }

    void swap(std::size_t a, std::size_t b) {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (
                [&](std::size_t _) {
                    using T = typeset::nth_t<Is, Ts...>;
                    std::swap(reinterpret_cast<T*>(vectors[Is])[a], reinterpret_cast<T*>(vectors[Is])[b]);
                }(Is),
                ...);
        }(std::index_sequence_for<Ts...>{});
    }

    std::size_t size() const {
        return _size;
    }

    std::size_t* unsafe_size_ptr() {
        return &_size;
    }

    std::size_t capacity() const {
        return _capacity;
    }

    template <typename T>
        requires typeset::in_set_v<T, Ts...>
    T& get(std::size_t i) {
        constexpr std::size_t ix = typeset::find_first_v<typeset::is_same_to<T>::template apply, Ts...>;

        return reinterpret_cast<T*>(vectors[ix])[i];
    };

    template <typeset::unique_v... Subset>
        requires(sizeof...(Subset) > 1 && typeset::is_subset_v<type_set<Subset...>, type_set<Ts...>>)
    std::tuple<Subset&...> get(std::size_t i) {
        constexpr std::array<std::size_t, sizeof...(Subset)> indexes = {
            typeset::find_first_v<typeset::is_same_to<Subset>::template apply, Ts...>...};

        return [&]<std::size_t... Is>(std::index_sequence<Is...>) -> std::tuple<Subset&...> {
            return {reinterpret_cast<typeset::nth_t<indexes[Is], Ts...>*>(vectors[indexes[Is]])[i]...};
        }(std::make_index_sequence<indexes.size()>());
    }

    template <typename T>
        requires typeset::in_set_v<T, Ts...>
    std::span<T> get_span() {
        constexpr std::size_t ix = typeset::find_first_v<typeset::is_same_to<T>::template apply, Ts...>;

        return std::span(reinterpret_cast<T*>(vectors[ix]), _size);
    }

    template <typeset::unique_v... Subset>
        requires(sizeof...(Subset) > 1 && typeset::is_subset_v<type_set<Subset...>, type_set<Ts...>>)
    std::tuple<std::span<Subset>...> get_span() {
        constexpr std::array<std::size_t, sizeof...(Subset)> indexes = {
            typeset::find_first_v<typeset::is_same_to<Subset>::template apply, Ts...>...};

        return [&]<std::size_t... Is>(std::index_sequence<Is...>) -> std::tuple<std::span<Subset>...> {
            return {std::span(reinterpret_cast<typeset::nth_t<indexes[Is], Ts...>*>(vectors[indexes[Is]]), _size)...};
        }(std::make_index_sequence<indexes.size()>());
    }

    template <typename T>
        requires typeset::in_set_v<T, Ts...>
    T* get_ptr(std::size_t i) {
        constexpr std::size_t ix = typeset::find_first_v<typeset::is_same_to<T>::template apply, Ts...>;

        return reinterpret_cast<T*>(&reinterpret_cast<T*>(vectors[ix])[i]);
    }

    template <typeset::unique_v... Subset>
        requires(sizeof...(Subset) > 1 && typeset::is_subset_v<type_set<Subset...>, type_set<Ts...>>)
    std::tuple<Subset*...> get_ptr(std::size_t i) {
        constexpr std::array<std::size_t, sizeof...(Subset)> indexes = {
            typeset::find_first_v<typeset::is_same_to<Subset>::template apply, Ts...>...};

        return [&]<std::size_t... Is>(std::index_sequence<Is...>) -> std::tuple<Subset*...> {
            return {reinterpret_cast<typeset::nth_t<indexes[Is], Ts...>*>(
                &reinterpret_cast<typeset::nth_t<indexes[Is], Ts...>*>(vectors[indexes[Is]])[i])...};
        }(std::make_index_sequence<indexes.size()>());
    }

    template <typename T>
        requires typeset::in_set_v<T, Ts...>
    T** get_ptr() {
        constexpr std::size_t ix = typeset::find_first_v<typeset::is_same_to<T>::template apply, Ts...>;

        return reinterpret_cast<T**>(&vectors[ix]);
    }

    template <typeset::unique_v... Subset>
        requires(sizeof...(Subset) > 1 && typeset::is_subset_v<type_set<Subset...>, type_set<Ts...>>)
    std::tuple<Subset**...> get_ptr() {
        constexpr std::array<std::size_t, sizeof...(Subset)> indexes = {
            typeset::find_first_v<typeset::is_same_to<Subset>::template apply, Ts...>...};

        return [&]<std::size_t... Is>(std::index_sequence<Is...>) -> std::tuple<Subset**...> {
            return {reinterpret_cast<typeset::nth_t<indexes[Is], Ts...>**>(&vectors[indexes[Is]])...};
        }(std::make_index_sequence<indexes.size()>());
    }

    void** get_dynamic(std::type_index t) {
        void** ptr = nullptr;
        ([&]<typename T>(std::in_place_type_t<T>) {
            if (typeid(T) != t) return false;

            ptr = get_ptr<T>();
            return true;
        }(std::in_place_type_t<Ts>{}) || ...);

        return ptr;
    }

    std::vector<void**> get_dynamic(const std::span<std::type_index>& subset) {
        std::vector<void**> res{};

        for (const auto& t : subset) {
            auto found = get_dynamic(t);
            assert(found != nullptr);

            res.emplace_back(found);
        }

        assert(res.size() == subset.size());
        return res;
    }

    template <typename T>
        requires typeset::in_set_v<T, Ts...>
    T& front() {
        return get<T>(0);
    };

    template <typeset::unique_v... Subset>
        requires(sizeof...(Subset) > 1 && typeset::is_subset_v<type_set<Subset...>, type_set<Ts...>>)
    std::tuple<Subset&...> front() {
        return get<Subset...>(0);
    }

    template <typename T>
        requires typeset::in_set_v<T, Ts...>
    T& back() {
        return get<T>(_size - 1);
    };

    template <typeset::unique_v... Subset>
        requires(sizeof...(Subset) > 1 && typeset::is_subset_v<type_set<Subset...>, type_set<Ts...>>)
    std::tuple<Subset&...> back() {
        return get<Subset...>(_size - 1);
    }

    void append(multi_vector& mv) {
        for (std::size_t i = 0; i < mv.size(); i++) {
            unsafe_push();

            [&]<std::size_t... Is>(std::index_sequence<Is...>) {
                (
                    [&](std::size_t _) {
                        using T = typeset::nth_t<Is, Ts...>;
                        std::construct_at(&reinterpret_cast<T*>(vectors[Is])[_size - 1],
                                          reinterpret_cast<T*>(mv.vectors[Is])[i]);
                    }(Is),
                    ...);
            }(std::index_sequence_for<Ts...>{});
        }
    }

    template <typename... Packs>
        requires(sizeof...(Packs) == sizeof...(Ts))
    void emplace_back(Packs&&... packs) {
        if (!can_insert()) resize(_capacity * 2);

        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (
                [&](std::size_t _) {
                    using T = typeset::nth_t<Is, Ts...>;

                    if constexpr (typeset::is_pack_v<typeset::nth_t<Is, Packs...>> &&
                                  !std::is_same_v<T, typeset::nth_t<Is, Packs...>>) {
                        typeset::with_pack(std::get<Is>(std::forward_as_tuple(std::forward<Packs>(packs)...)),
                                           [&](auto... args) {
                                               std::construct_at(&reinterpret_cast<T*>(vectors[Is])[_size], args...);
                                           });
                    } else {
                        std::construct_at(&reinterpret_cast<T*>(vectors[Is])[_size],
                                          std::get<Is>(std::forward_as_tuple(std::forward<Packs>(packs)...)));
                    }
                }(Is),
                ...);
        }(std::index_sequence_for<Ts...>{});
        _size++;
    }

    std::tuple<Ts*...> unsafe_push() {
        if (!can_insert()) resize(_capacity * 2);

        _size++;

        return get_ptr<Ts...>(_size - 1);
    }

    template <typename... Packs>
        requires(sizeof...(Packs) == sizeof...(Ts))
    void emplace_at(std::size_t i, Packs&&... packs) {
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (
                [&](std::size_t _) {
                    using T = typeset::nth_t<Is, Ts...>;

                    std::destroy_at(&reinterpret_cast<T*>(vectors[Is])[i]);

                    if constexpr (typeset::is_pack_v<typeset::nth_t<Is, Packs...>> &&
                                  !std::is_same_v<T, typeset::nth_t<Is, Packs...>>) {
                        typeset::with_pack(
                            std::get<Is>(std::forward_as_tuple(std::forward<Packs>(packs)...)),
                            [&](auto... args) { std::construct_at(&reinterpret_cast<T*>(vectors[Is])[i], args...); });
                    } else {
                        std::construct_at(&reinterpret_cast<T*>(vectors[Is])[i],
                                          std::get<Is>(std::forward_as_tuple(std::forward<Packs>(packs)...)));
                    }
                }(Is),
                ...);
        }(std::index_sequence_for<Ts...>{});
    }

    void pop_back() {
        if (_size == 0) return;

        _size--;
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (std::destroy_at(&reinterpret_cast<typeset::nth_t<Is, Ts...>*>(vectors[Is])[_size]), ...);
        }(std::index_sequence_for<Ts...>{});
    }

  private:
    std::size_t _capacity;
    std::size_t _size = 0;
    std::array<void*, sizeof...(Ts)> vectors;

    bool can_insert() const {
        return _capacity > _size;
    }

    void resize(std::size_t new_capacity) {
        void* tmp_vec;
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (
                [&](std::size_t _) {
                    using T = typeset::nth_t<Is, Ts...>;

                    tmp_vec = ::operator new(new_capacity * sizeof(T));
                    for (std::size_t i = 0; i < _size; i++) {
                        if constexpr (std::move_constructible<T>) {
                            std::construct_at(&reinterpret_cast<T*>(tmp_vec)[i],
                                              std::move(reinterpret_cast<T*>(vectors[Is])[i]));
                        } else {
                            std::construct_at(&reinterpret_cast<T*>(tmp_vec)[i], reinterpret_cast<T*>(vectors[Is])[i]);
                        }

                        std::destroy_at(&reinterpret_cast<T*>(vectors[Is])[i]);
                    }

                    ::operator delete(vectors[Is]);
                    vectors[Is] = tmp_vec;
                }(Is),
                ...);
        }(std::index_sequence_for<Ts...>{});

        _capacity = new_capacity;
    }
};
