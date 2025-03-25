#pragma once

#include "print"
#include "utility.hpp"
#include <algorithm>
#include <cassert>
#include <memory>
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
            static std::uniform_real_distribution<float> distPhi(0.0f, std::numbers::pi / 2.0f);
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

namespace particle_system_2 {
    struct Particles {
        std::unique_ptr<Vector3[]> pos;
        std::unique_ptr<Vector3[]> velocity;
        std::unique_ptr<Vector3[]> acceleration;
        std::unique_ptr<Color[]> color;
        std::unique_ptr<bool[]> alive;
        std::unique_ptr<float[]> lifetime;

        std::size_t size;
        std::size_t alive_count = 0;

        Particles(std::size_t max_particles) : size(max_particles) {
            pos.reset(new Vector3[max_particles]);
            velocity.reset(new Vector3[max_particles]);
            acceleration.reset(new Vector3[max_particles]);
            color.reset(new Color[max_particles]);
            alive.reset(new bool[max_particles]);
            lifetime.reset(new float[max_particles]);
        }

        Particles(const Particles&) = delete;
        Particles& operator=(const Particles&) = delete;

        void kill(std::size_t ix) {
            if (alive_count <= 0) return;

            alive[ix] = false;
            swap(ix, --alive_count);
        }

        void wake(std::size_t ix) {
            if (alive_count >= size) return;

            std::println("WAKY WAKY");
            alive[ix] = true;
            swap(ix, alive_count++);
        }

        void swap(std::size_t a, std::size_t b) {
            std::swap(pos[a], pos[b]);
            std::swap(velocity[a], velocity[b]);
            std::swap(acceleration[a], acceleration[b]);
            std::swap(color[a], color[b]);
            std::swap(alive[a], alive[b]);
            std::swap(lifetime[a], lifetime[b]);
        }

        void debug(std::size_t i) {
            auto print_vec3 = [](Vector3 v) {
                std::print("[{}, {}, {}]", v.x, v.y, v.z);
            };

            std::print("POS: ");
            print_vec3(pos[i]);
            std::print("\nVELOCITY: ");
            print_vec3(velocity[i]);
            std::print("\nACCEL: ");
            print_vec3(acceleration[i]);
            std::print("\nCOLOR: ");
            std::println("[{}, {}, {}, {}]", color[i].r, color[i].g, color[i].b, color[i].a);
            std::println("\nALIVE: {}", alive[i]);
            std::println("\nIFETIME: {}", lifetime[i]);
        }
    };

    namespace generators {
        struct DefaultInicialize {
            void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
                for (std::size_t i = start_ix; i < end_ix; i++) {
                    particles.pos[i] = Vector3Zero();
                    particles.velocity[i] = Vector3Zero();
                    particles.acceleration[i] = Vector3Zero();
                    particles.color[i] = Color{ 0, 0, 0, 0};
                    particles.alive[i] = false;
                    particles.lifetime[i] = 0.0f;
                }
            }
        };

        namespace pos {
            struct FixedPoint {
                Vector3 origin;

                FixedPoint(Vector3 origin) : origin(origin) {
                }

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
                    for (std::size_t i = start_ix; i < end_ix; i++) {
                        particles.pos[i] = origin;
                    }
                }
            };

            struct OnCircle {
                Vector3 origin;
                float radius;

                OnCircle(Vector3 origin, float radius) : origin(origin), radius(radius) {
                }

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
                }
            };
        }

        namespace velocity {
            struct Sphere {
                bool half_sphere;
                float speed;

                Sphere(bool half_sphere, float speed) : half_sphere(half_sphere), speed(speed) {
                }

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
                    static std::uniform_real_distribution<float> distTheta(0.0f, 2 * std::numbers::pi);
                    static std::uniform_real_distribution<float> distPhiHalf(0.0f, std::numbers::pi);
                    static std::uniform_real_distribution<float> distPhi(0.0f, std::numbers::pi / 2.0f);
                    auto& gen = rng::get();

                    for (std::size_t i = start_ix; i < end_ix; i++) {
                        float theta = distTheta(gen);
                        float phi = half_sphere ? distPhiHalf(gen) : distPhi(gen);

                        particles.velocity[i] = {
                            .x = speed * std::sin(phi) * std::cos(theta),
                            .y = speed * std::cos(phi),
                            .z = speed * std::sin(phi) * std::sin(theta),
                        };
                    }
                }
            };

            struct SpeedRange {
                float min;
                float max;
                std::uniform_real_distribution<float> dist;

                SpeedRange(float min, float max) : min(min), max(max), dist(min, max) {}

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
                    auto& gen = rng::get();
                    for (std::size_t i = start_ix; i < end_ix; i++) {
                        particles.velocity[i] = Vector3Scale(particles.velocity[i], dist(gen));
                    }
                }
            };
        }

        namespace color {
            struct Fixed {
                Color color;

                Fixed(Color color) : color(color) {}

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
                    for (std::size_t i = start_ix; i < end_ix; i++) {
                        particles.color[i] = color;
                    }
                }
            };
        }

        namespace lifetime {
            struct Range {
                float min;
                float max;
                std::uniform_real_distribution<float> dist;

                Range(float min, float max) : min(min), max(max), dist(min, max) {
                }

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
                    auto& gen = rng::get();

                    for (std::size_t i = start_ix; i < end_ix; i++) {
                        particles.lifetime[i] = dist(gen);
                    }
                }
            };
        }

        using Generator = std::variant<pos::FixedPoint, pos::OnCircle, velocity::Sphere, velocity::SpeedRange, color::Fixed, lifetime::Range>;
    };

    namespace emitters {
        struct CustomEmitter {
            std::vector<generators::Generator> generators;
            float emit_rate;

            CustomEmitter(float emit_rate) : emit_rate(emit_rate) {
            }

            CustomEmitter(const CustomEmitter&) = delete;
            CustomEmitter operator=(const CustomEmitter&) = delete;

            CustomEmitter(CustomEmitter&&) noexcept = default;
            CustomEmitter& operator=(const CustomEmitter&&) noexcept = delete;

            void emit(Particles& particles, float dt) {
                std::size_t max_particles = (std::size_t)std::floor(dt * emit_rate);
                std::size_t start_ix = particles.alive_count;
                std::size_t end_ix = std::min(start_ix + max_particles, particles.size - 1);

                for (auto& gen : generators) {
                    std::visit([&](auto&& arg) { arg.gen(particles, dt, start_ix, end_ix); }, gen);
                }

                std::println("BEFORE WAKING: {}<->{}, {}, {} * {} = {}", start_ix, end_ix, max_particles, dt, emit_rate, std::floor(dt * emit_rate));
                for (std::size_t i = start_ix; i < end_ix; i++) {
                    particles.wake(i);
                }
            }

            void add_generator(generators::Generator&& gen) {
                generators.emplace_back(std::forward<generators::Generator>(gen));
            }
        };

        using Emitter = std::variant<CustomEmitter>;
    }

    namespace updaters {
        struct Lifetime {
            void update(Particles& particles, float dt) {
                auto end_ix = particles.alive_count;

                for (std::size_t i = 0; i < end_ix; i++) {
                    particles.lifetime[i] -= dt;

                    if (particles.lifetime[i] <= 0.0f) {
                        particles.kill(i);
                        end_ix = particles.alive_count < particles.size ? particles.alive_count : particles.size;
                    }
                }
            }
        };

        struct Position {
            void update(Particles& particles, float dt) {
                for (std::size_t i = 0; i < particles.alive_count; i++) {
                    particles.velocity[i] =
                        Vector3Add(particles.velocity[i], Vector3Scale(particles.acceleration[i], dt));
                    particles.pos[i] = Vector3Add(particles.pos[i], Vector3Scale(particles.velocity[i], dt));
                }
            }
        };

        using Updater = std::variant<Lifetime, Position>;
    }

    struct System {
        std::vector<emitters::Emitter> emitters;
        std::vector<updaters::Updater> updaters;

        Particles particles;

        System(std::size_t max_particles) : particles(max_particles) {
        }

        void update(float dt) {
            for (auto& e : emitters) {
                std::visit([&](auto&& arg) { arg.emit(particles, dt); }, e);
            }

            /*for (std::size_t i = 0; i < particles.alive_count; i++) {*/
            /*    particles.debug(i);*/
            /*}*/
            assert(false);

            for (auto& u : updaters) {
                std::visit([&](auto&& arg) { arg.update(particles, dt); }, u);
            }
        }

        void draw() {
            for (std::size_t i = 0; i < particles.alive_count; i++) {
                DrawSphere(particles.pos[i], 1.0f, particles.color[i]);
            }
        }

        void reset() {
            particles.alive_count = 0;
        }

        void add_emitter(emitters::Emitter&& emitter) {
            emitters.emplace_back(std::forward<emitters::Emitter>(emitter));
        }

        void add_updater(updaters::Updater&& updater) {
            updaters.emplace_back(std::forward<updaters::Updater>(updater));
        }
    };
}
