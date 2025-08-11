#pragma once

#include <glm/common.hpp>
#include <glm/ext/vector_float3.hpp>

namespace engine::collisions {
    struct AABB {
        glm::vec3 min, max;

        AABB() : min{0.0f}, max{0.0f} {};
        AABB(glm::vec3 min, glm::vec3 max) : min(min), max(max) {};

        void extend(const AABB& other) {
            if (min == glm::vec3{0.0f} && max == glm::vec3{0.0f}) {
                min = other.min;
                max = other.max;
            } else {
                min = glm::min(min, other.min);
                max = glm::max(max, other.max);
            }
        }
    };
};
