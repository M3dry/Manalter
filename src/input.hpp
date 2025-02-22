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
    void unregister_key(int key);
    void unregister_all();

    void poll();
    void tick(std::function<void(const int&)> key_handler);
};

class Mouse {
    enum struct Button {
        Left,
        Right,
    };

    struct State {
        Button state;
        Vector2 pressed_at;
        std::optional<Vector2> released_at;
    };

    Vector2 mouse_pos;
    std::optional<State> keypress;

    void poll();
};
