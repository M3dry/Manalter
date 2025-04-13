#pragma once

#include <functional>
#include <optional>
#include <raylib.h>
#include <vector>

class Keys {
  private:
    std::vector<int> registered;
    std::vector<std::pair<int, bool /* if handled */>> pressed;

    bool is_pressed(int key) const;

  public:
    void register_key(int key);
    void register_all();
    void unregister_key(int key);
    void unregister_all();

    void handled_key(int key);

    void poll();
    void tick(std::function<void(const int&)> key_handler);
};

struct Mouse {
    struct Dummy {};

    enum struct Button {
        Left,
        Right,
    };

    struct State {
        Button button;
        Vector2 pressed_at;
        std::optional<Vector2> released_at;
    };

    std::optional<State> button_press = std::nullopt;
    Vector2 mouse_pos = {-1.0f, -1.0f};
    float wheel_movement = 0.0f;

    Mouse();
    template <typename T>
        requires std::same_as<T, Dummy>
    Mouse(T) {
    }

    void poll();
};
