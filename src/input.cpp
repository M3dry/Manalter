#include "input.hpp"

#include <algorithm>
#include <raylib.h>

bool Keys::is_pressed(int key) const {
    for (const auto& [k, _] : pressed) {
        if (k == key) return true;
    }

    return false;
}

void Keys::register_key(int key) {
    registered.emplace_back(key);
}

void Keys::unregister_key(int key) {
    auto [first, last] = std::ranges::remove_if(registered, [&key](const auto& k) {
        return k == key;
    });
    registered.erase(first, last);
}

void Keys::unregister_all() {
    registered.clear();
    registered.shrink_to_fit();
}

void Keys::poll() {
    auto [first, last] = std::ranges::remove_if(pressed, [](const auto& pair) {
        return pair.second && IsKeyUp(pair.first);
    });
    pressed.erase(first, last);

    for (const auto& key : registered) {
        if (is_pressed(key)) continue;
        if (IsKeyDown(key)) pressed.emplace_back(key, false);
    }
}

void Keys::tick(std::function<void(const int&)> key_handler) {
    for (auto& [key, handled] : pressed) {
        if (!handled) {
            key_handler(key);
            handled = true;
        }
    }
}
