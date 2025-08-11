#pragma once

#include "ecs.hpp"

#define MAKE_WRAPPED_ID(name)                                                                                          \
    class name {                                                                                                       \
      public:                                                                                                          \
        explicit name(ecs::Entity e) : id(e) {}                                                                        \
        operator ecs::Entity() const {                                                                                 \
            return id;                                                                                                 \
        }                                                                                                              \
                                                                                                                       \
      private:                                                                                                         \
        ecs::Entity id;                                                                                                \
    };
