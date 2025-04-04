#pragma once

#include "print"
#include "utility.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <random>
#include <raylib.h>
#include <raymath.h>
#include <vector>

#include "glad.h"
#include "rlgl.h"

namespace particle_system {
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
            pos.reset(new Vector3[max_particles]{});
            velocity.reset(new Vector3[max_particles]{});
            acceleration.reset(new Vector3[max_particles]{});
            color.reset(new Color[max_particles]{});
            alive.reset(new bool[max_particles]{});
            lifetime.reset(new float[max_particles]{});
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
    };

    namespace generators {
        namespace pos {
            struct Fixed {
                Vector3 origin;

                Fixed(Vector3 origin) : origin(origin) {
                }

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
                    for (std::size_t i = start_ix; i < end_ix; i++) {
                        particles.pos[i] = origin;
                    }
                }
            };

            struct OnCircle {
                Vector3 center;
                float radius;

                OnCircle(Vector3 center, float radius) : center(center), radius(radius) {
                }

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
                    static std::uniform_real_distribution<float> distAngle(0.0f, 2 * std::numbers::pi);

                    auto& gen = rng::get();
                    for (std::size_t i = start_ix; i < end_ix; i++) {
                        particles.pos[i].x = radius * std::cos(distAngle(gen));
                        particles.pos[i].y = radius * std::sin(distAngle(gen));
                    }
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
                    static std::uniform_real_distribution<float> distPhiHalf(0.0f, std::numbers::pi / 2.0f);
                    static std::uniform_real_distribution<float> distPhi(0.0f, std::numbers::pi);
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

            // vecolity must already be set
            struct ScaleRange {
                float min;
                float max;
                std::uniform_real_distribution<float> dist;

                ScaleRange(float min, float max) : min(min), max(max), dist(min, max) {
                }

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
                    auto& gen = rng::get();
                    for (std::size_t i = start_ix; i < end_ix; i++) {
                        particles.velocity[i] = Vector3Scale(particles.velocity[i], dist(gen));
                    }
                }
            };
        }

        namespace acceleration {
            struct Fixed {
                Vector3 accel;

                Fixed(Vector3 accel) : accel(accel) {
                }

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
                    for (std::size_t i = start_ix; i < end_ix; i++) {
                        particles.acceleration[i] = accel;
                    }
                }
            };

            struct Uniform {
                Vector3 magnitude;

                Uniform(float magnitude) : magnitude({magnitude, magnitude, magnitude}) {
                }
                Uniform(Vector3 magnitude) : magnitude(magnitude) {
                }

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
                    for (std::size_t i = start_ix; i < end_ix; i++) {
                        auto normalized = Vector3Normalize(particles.velocity[i]);
                        particles.acceleration[i].x = normalized.x * magnitude.x;
                        particles.acceleration[i].y = normalized.y * magnitude.y;
                        particles.acceleration[i].z = normalized.z * magnitude.z;
                    }
                }
            };
        }

        namespace color {
            struct Fixed {
                Color color;

                Fixed(Color color) : color(color) {
                }

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

        using AnonGen = std::function<void(Particles&, float, std::size_t, std::size_t)>;
        using Generator =
            std::variant<pos::Fixed, pos::OnCircle, velocity::Sphere, velocity::ScaleRange, acceleration::Fixed,
                         acceleration::Uniform, color::Fixed, lifetime::Range, AnonGen>;
    };

    namespace emitters {
        template <bool Reemit = false> struct CustomEmitter {
            std::vector<generators::Generator> generators;
            float emit_rate;
            bool emitted = false;

            CustomEmitter(float emit_rate = std::numeric_limits<float>::max()) : emit_rate(emit_rate) {
            }

            CustomEmitter(const CustomEmitter&) = delete;
            CustomEmitter operator=(const CustomEmitter&) = delete;

            CustomEmitter(CustomEmitter&&) noexcept = default;
            CustomEmitter& operator=(const CustomEmitter&&) noexcept = delete;

            void emit(Particles& particles, float dt) {
                if (emitted) return;
                if (!Reemit) emitted = true;

                float max_particles = std::floor(dt * emit_rate);
                if (max_particles >= (float)std::numeric_limits<std::size_t>::max())
                    max_particles = (float)std::numeric_limits<std::size_t>::max();
                if (max_particles <= 0) max_particles = 0;
                std::size_t start_ix = particles.alive_count;
                std::size_t end_ix = std::min(start_ix + (std::size_t)max_particles, particles.size - 1);

                for (auto& gen : generators) {
                    std::visit(
                        [&](auto&& arg) {
                            using T = std::decay_t<decltype(arg)>;
                            if constexpr (std::is_same_v<T, generators::AnonGen>) {
                                arg(particles, dt, start_ix, end_ix);
                            } else {
                                arg.gen(particles, dt, start_ix, end_ix);
                            }
                        },
                        gen);
                }

                for (std::size_t i = start_ix; i < end_ix; i++) {
                    particles.wake(i);
                }
            }

            void add_generator(generators::Generator&& gen) {
                generators.emplace_back(std::forward<generators::Generator>(gen));
            }
        };

        using Emitter = std::variant<CustomEmitter<false>, CustomEmitter<true>>;
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

        struct ColorByVelocity {
            Color start_col;
            Color end_col;
            float min_threshold;
            float max_threshold;

            ColorByVelocity(Color start, Color end, float min_threshold, float max_threshold)
                : start_col(start), end_col(end), min_threshold(min_threshold), max_threshold(max_threshold) {
            }

            void update(Particles& particles, float dt) {
                for (std::size_t i = 0; i < particles.alive_count; i++) {
                    auto speed = std::abs(Vector3Length(particles.velocity[i]));
                    auto factor = (speed - min_threshold) / (max_threshold - min_threshold);

                    if (factor < 0.0f) {
                        particles.color[i] = lerp_color(particles.color[i], start_col, std::abs(speed / min_threshold));
                    } else {
                        particles.color[i] = lerp_color(start_col, end_col, factor);
                    }
                }
            }
        };

        using AnonUpdater = std::function<void(Particles& particles, float dt)>;
        using Updater = std::variant<Lifetime, Position, ColorByVelocity, AnonUpdater>;
    }

    struct System {
        static constexpr std::size_t pos_vertex_size = 3 * sizeof(float);
        static constexpr std::size_t col_vertex_size = 4 * sizeof(uint8_t);

        std::vector<emitters::Emitter> emitters;
        std::vector<updaters::Updater> updaters;

        Particles particles;

        std::unique_ptr<uint8_t[]> pos_vertex_data;
        std::unique_ptr<uint8_t[]> col_vertex_data;
        unsigned int vao_id = 0;
        unsigned int pos_vbo_id = 0;
        unsigned int col_vbo_id = 0;
        Shader shader;

        System(std::size_t max_particles)
            : particles(max_particles), vao_id(rlLoadVertexArray()),
              pos_vbo_id(rlLoadVertexBuffer(NULL, pos_vertex_size * max_particles, true)),
              col_vbo_id(rlLoadVertexBuffer(NULL, col_vertex_size * max_particles, true)),
              shader(LoadShader("./assets/particles.vs.glsl", "./assets/particles.fs.glsl")) {
            pos_vertex_data.reset(new uint8_t[max_particles * pos_vertex_size]{});
            col_vertex_data.reset(new uint8_t[max_particles * col_vertex_size]{});

            rlEnableVertexArray(vao_id);

            rlEnableVertexBuffer(pos_vbo_id);
            rlEnableVertexAttribute(0);
            rlSetVertexAttribute(0, 3, RL_FLOAT, false, pos_vertex_size, 0);

            rlEnableVertexBuffer(col_vbo_id);
            rlEnableVertexAttribute(1);
            rlSetVertexAttribute(1, 4, RL_UNSIGNED_BYTE, true, col_vertex_size, 0);

            rlDisableVertexArray();
            rlDisableVertexBuffer();
        }

        ~System() {
            rlUnloadVertexArray(vao_id);
            rlUnloadVertexBuffer(pos_vbo_id);
            rlUnloadVertexBuffer(col_vbo_id);
        }

        void update(float dt) {
            for (auto& e : emitters) {
                std::visit([&](auto&& arg) { arg.emit(particles, dt); }, e);
            }

            for (auto& u : updaters) {
                std::visit([&](auto&& arg) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, updaters::AnonUpdater>) {
                        arg(particles, dt);
                    } else {
                        arg.update(particles, dt);
                    }
                }, u);
            }
        }

        void draw() {
            if (particles.alive_count == 0) return;

            std::memcpy(pos_vertex_data.get(), particles.pos.get(), pos_vertex_size * particles.alive_count);
            std::memcpy(col_vertex_data.get(), particles.color.get(), col_vertex_size * particles.alive_count);

            rlUpdateVertexBuffer(pos_vbo_id, pos_vertex_data.get(), pos_vertex_size * particles.alive_count, 0);
            rlUpdateVertexBuffer(col_vbo_id, col_vertex_data.get(), col_vertex_size * particles.alive_count, 0);

            auto mvp = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
            auto loc = GetShaderLocation(shader, "mvp");

            SetShaderValueMatrix(shader, loc, mvp);
            glEnable(GL_PROGRAM_POINT_SIZE);
            glUseProgram(shader.id);
            rlEnableVertexArray(vao_id);
            glDrawArrays(GL_POINTS, 0, particles.alive_count);
            rlDisableVertexArray();
            glUseProgram(rlGetShaderIdDefault());
            glDisable(GL_PROGRAM_POINT_SIZE);
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
