#pragma once

#include <functional>
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
