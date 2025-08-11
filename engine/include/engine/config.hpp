#pragma once

namespace engine::config {
#ifdef ENGINE_MODE
    inline constexpr bool debug_mode = true;
#else
    inline constexpr bool debug_mode = false;
#endif
}
