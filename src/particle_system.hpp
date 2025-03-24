#pragma once

#include "utility.hpp"
#include <algorithm>
#include <random>
#include <raylib.h>
#include <raymath.h>
#include <vector>

namespace particle_system {
    struct Particle {
        Vector3 pos;
        Vector3 velocity;
        Vector3 accel = Vector3Zero();
        Color color;

        // in seconds
        float lifetime;
        float size;

        Particle(const Vector3 pos, const Vector3 vel, const Color& col, float lifetime, float size)
            : pos(pos), velocity(vel), color(col), lifetime(lifetime), size(size) {
        }
    };

    struct Emitter {
        std::vector<Particle> particles;

        void emit_explosion(const Vector3& origin, std::size_t particle_count) {
            auto& gen = rng::get();

            static std::uniform_real_distribution<float> distTheta(0.0f, 2 * std::numbers::pi);
            static std::uniform_real_distribution<float> distPhi(0.0f, std::numbers::pi/ 2.0f);
            static std::uniform_real_distribution<float> distSpeed(5.f, 15.f);
            static std::uniform_real_distribution<float> distLife(0.5f, 1.5f);
            static std::uniform_real_distribution<float> distColor(128, 255);

            for (std::size_t i = 0; i < particle_count; i++) {
                float theta = distTheta(gen);
                float phi = distPhi(gen);
                float speed = distSpeed(gen);

                Vector3 velocity = {
                    .x = speed * std::sin(phi) * std::cos(theta),
                    .y = speed * std::cos(phi),
                    .z = speed * std::sin(phi) * std::sin(theta),
                };

                Color color = {
                    .r = (uint8_t)distColor(gen),
                    .g = (uint8_t)(distColor(gen) * 0.5f),
                    .b = 0,
                    .a = 255,
                };

                float life = distLife(gen);
                float size = 1.0f;

                particles.emplace_back(origin, velocity, color, life, size);
            }
        }

        void update(float dt) {
            for (auto& p : particles) {
                p.velocity = Vector3Add(p.velocity, Vector3Scale(p.accel, dt));
                p.pos = Vector3Add(p.pos, Vector3Scale(p.velocity, dt));
                p.lifetime -= dt;
            }

            particles.erase(std::remove_if(particles.begin(), particles.end(),
                                           [](const auto& p) -> bool { return p.lifetime <= 0.0f; }),
                            particles.end());
        }

        void draw() {
            for (const auto& p : particles) {
                DrawSphere(p.pos, p.size, p.color);
            }
        }
    };
}
