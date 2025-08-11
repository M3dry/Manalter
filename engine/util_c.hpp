#pragma once

#include <format>
#include <memory>
#include <print>

template <typename... Ts> void abort(std::format_string<Ts...> str, Ts&&... message) {
    std::println(str, std::forward<Ts>(message)...);
}

template <typename T>
struct Deleter {
    void operator()(T* t) {
        ::operator delete(t);
    }
};

template <typename T>
using array_unique_ptr = std::unique_ptr<T[], Deleter<T>>;
