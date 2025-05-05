#pragma once

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
        std::unique_ptr<float[]> size;

        std::size_t max_size;
        std::size_t alive_count = 0;

        Particles(std::size_t max_particles);
        Particles(Particles&&) noexcept = default;
        Particles& operator=(Particles&&) noexcept = default;

        void kill(std::size_t ix);
        void wake(std::size_t ix);
        void swap(std::size_t a, std::size_t b);
    };

    namespace generators {
        namespace pos {
            struct Fixed {
                Vector3 origin;

                Fixed(Vector3 origin);

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix);
            };

            struct OnCircle {
                Vector3 center;
                std::uniform_real_distribution<float> radiusDist;

                OnCircle(Vector3 center, std::pair<float, float> radius);

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix);
            };

            struct OnSphere {
                Vector3 center;
                std::uniform_real_distribution<float> radiusDist;

                OnSphere(Vector3 center, std::pair<float, float> radius);

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix);
            };
        }

        namespace velocity {
            struct Sphere {
                float speed;
                std::optional<Vector3> hemisphere_vec;

                Sphere(float speed, std::optional<Vector3> hemisphere_vec = std::nullopt);

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix);
            };

            // vecolity must already be set
            struct ScaleRange {
                float min;
                float max;
                std::uniform_real_distribution<float> dist;

                ScaleRange(float min, float max);

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix);
            };
        }

        namespace acceleration {
            struct Fixed {
                Vector3 accel;

                Fixed(Vector3 accel);

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix);
            };

            struct Uniform {
                Vector3 magnitude;

                Uniform(float magnitude);
                Uniform(Vector3 magnitude);

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix);
            };
        }

        namespace color {
            struct Fixed {
                Color color;

                Fixed(Color color);

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix);
            };
        }

        namespace lifetime {
            struct Range {
                float min;
                float max;
                std::uniform_real_distribution<float> dist;

                Range(float min, float max);

                void gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix);
            };
        }

        using AnonGen = std::function<void(Particles&, float, std::size_t, std::size_t)>;
        using Generator =
            std::variant<pos::Fixed, pos::OnCircle, pos::OnSphere, velocity::Sphere, velocity::ScaleRange,
                         acceleration::Fixed, acceleration::Uniform, color::Fixed, lifetime::Range, AnonGen>;
    };

    namespace emitters {
        struct CustomEmitter {
            std::vector<generators::Generator> generators;
            float emit_rate;
            std::size_t max_emit;

            CustomEmitter(float emit_rate = static_cast<float>(std::numeric_limits<std::size_t>::max()),
                          std::size_t max_emit = std::numeric_limits<std::size_t>::max())
                : emit_rate(emit_rate), max_emit(max_emit) {
            }

            CustomEmitter(const CustomEmitter&) = delete;
            CustomEmitter& operator=(const CustomEmitter&) = delete;

            CustomEmitter(CustomEmitter&&) noexcept = default;
            CustomEmitter& operator=(CustomEmitter&&) noexcept = default;

            bool emit(Particles& particles, float dt) {
                std::size_t max_particles =
                    std::max<std::size_t>(std::min(static_cast<std::size_t>(dt * emit_rate), particles.max_size), 0);

                if (max_emit <= max_particles) {
                    max_particles = max_emit;
                    max_emit = 0;
                } else {
                    max_emit -= max_particles;
                }

                if (max_particles == 0) return max_emit != 0;

                std::size_t start_ix = particles.alive_count;
                std::size_t end_ix =
                    std::min(start_ix + static_cast<std::size_t>(max_particles), particles.max_size - 1);

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

                return max_emit != 0;
            }

            void add_generator(generators::Generator&& gen) {
                generators.emplace_back(std::forward<generators::Generator>(gen));
            }
        };

        using Emitter = std::variant<CustomEmitter>;
    }

    namespace updaters {
        struct Lifetime {
            void update(Particles& particles, float dt);
        };

        struct Position {
            void update(Particles& particles, float dt);
        };

        struct ColorByVelocity {
            Color start_col;
            Color end_col;
            float min_threshold;
            float max_threshold;

            ColorByVelocity(Color start, Color end, float min_threshold, float max_threshold);

            void update(Particles& particles, float dt);
        };

        using AnonUpdater = std::function<void(Particles& particles, float dt)>;
        using Updater = std::variant<Lifetime, Position, ColorByVelocity, AnonUpdater>;
    }

    namespace renderers {
        struct Point {
            static constexpr std::size_t pos_vertex_size = 3 * sizeof(float);
            static constexpr std::size_t size_vertex_size = sizeof(float);
            static constexpr std::size_t col_vertex_size = 4 * sizeof(uint8_t);

            Vector3 pos_offset = Vector3Zero();

            Texture2D particle_circle;

            std::size_t max_size;
            std::unique_ptr<uint8_t[]> pos_vertex_data;
            std::unique_ptr<uint8_t[]> size_vertex_data;
            std::unique_ptr<uint8_t[]> col_vertex_data;
            unsigned int vao_id = 0;
            unsigned int pos_vbo_id = 0;
            unsigned int size_vbo_id = 0;
            unsigned int col_vbo_id = 0;
            Shader shader;

            Point(std::size_t max_size, Vector3 pos_offset = Vector3Zero());
            Point(const Point&) = delete;
            Point& operator=(const Point&) = delete;
            Point(Point&& p) noexcept
                : pos_offset(p.pos_offset), particle_circle(p.particle_circle), max_size(p.max_size),
                  pos_vertex_data(std::move(p.pos_vertex_data)), size_vertex_data(std::move(p.size_vertex_data)),
                  col_vertex_data(std::move(p.col_vertex_data)), vao_id(p.vao_id), pos_vbo_id(p.pos_vbo_id),
                  size_vbo_id(p.size_vbo_id), col_vbo_id(p.col_vbo_id), shader(p.shader) {
                p.particle_circle.id = 0;
                p.vao_id = 0;
                p.pos_vbo_id = 0;
                p.size_vbo_id = 0;
                p.col_vbo_id = 0;
                p.shader.id = 0;
            };
            Point& operator=(Point&& p) noexcept {
                if (this != &p) {
                    pos_offset = p.pos_offset;
                    particle_circle = p.particle_circle;
                    max_size = p.max_size;
                    pos_vertex_data = std::move(p.pos_vertex_data);
                    size_vertex_data = std::move(p.size_vertex_data);
                    col_vertex_data = std::move(p.col_vertex_data);
                    vao_id = p.vao_id;
                    pos_vbo_id = p.pos_vbo_id;
                    size_vbo_id = p.size_vbo_id;
                    col_vbo_id = p.col_vbo_id;
                    shader = p.shader;
                    p.particle_circle.id = 0;
                    p.vao_id = 0;
                    p.pos_vbo_id = 0;
                    p.size_vbo_id = 0;
                    p.col_vbo_id = 0;
                    p.shader.id = 0;
                }

                return *this;
            };
            ~Point();

            void operator()(Particles& particles);
            void setup_gl();
        };

        using AnonRenderer = std::function<void(Particles& particles)>;
        using Renderer = std::variant<Point, AnonRenderer>;
    }

    struct System {
        std::optional<std::size_t> reset_on_done = std::nullopt;
        std::vector<emitters::Emitter> emitters;
        std::vector<updaters::Updater> updaters;
        renderers::Renderer renderer;

        Particles particles;

        System(std::size_t max_particles, renderers::Renderer&& renderer);
        System(System&& s) noexcept = default;
        System& operator=(System&& s) noexcept = default;

        // false - simulation ended
        bool update(float dt);
        void draw();
        void reset();
        void reset(std::optional<std::size_t> max_emit);
        void add_emitter(emitters::Emitter&& emitter);
        void add_updater(updaters::Updater&& updater);
    };
}
