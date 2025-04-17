#include "particle_system.hpp"
#include "utility.hpp"
#include <random>
#include <raylib.h>
#include <raymath.h>
#include <variant>

namespace particle_system {
    Particles::Particles(std::size_t max_particles) : max_size(max_particles) {
        pos.reset(new Vector3[max_size]{});
        velocity.reset(new Vector3[max_size]{});
        acceleration.reset(new Vector3[max_size]{});
        color.reset(new Color[max_size]{});
        alive.reset(new bool[max_size]{});
        lifetime.reset(new float[max_size]{});
        size.reset(new float[max_size]{});
    }

    void Particles::kill(std::size_t ix) {
        if (alive_count <= 0) return;

        alive[ix] = false;
        swap(ix, --alive_count);
    }

    void Particles::wake(std::size_t ix) {
        if (alive_count >= max_size) return;

        alive[ix] = true;
        swap(ix, alive_count++);
    }

    void Particles::swap(std::size_t a, std::size_t b) {
        std::swap(pos[a], pos[b]);
        std::swap(velocity[a], velocity[b]);
        std::swap(acceleration[a], acceleration[b]);
        std::swap(color[a], color[b]);
        std::swap(alive[a], alive[b]);
        std::swap(lifetime[a], lifetime[b]);
        std::swap(size[a], size[b]);
    }
}

namespace particle_system::generators::pos {
    Fixed::Fixed(Vector3 origin) : origin(origin) {
    }

    void Fixed::gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
        for (std::size_t i = start_ix; i < end_ix; i++) {
            particles.pos[i] = origin;
        }
    }

    OnCircle::OnCircle(Vector3 center, std::pair<float, float> radius) : center(center), radiusDist(radius.first, radius.second) {
    }

    void OnCircle::gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
        static std::uniform_real_distribution<float> distAngle(0.0f, 2 * static_cast<float>(std::numbers::pi));

        auto& gen = rng::get();
        for (std::size_t i = start_ix; i < end_ix; i++) {
            auto angle = distAngle(gen);
            auto radius = radiusDist(gen);
            particles.pos[i].x = center.x + radius * std::cos(angle);
            particles.pos[i].y = center.y;
            particles.pos[i].z = center.z + radius * std::sin(angle);
        }
    }

    OnSphere::OnSphere(Vector3 center, std::pair<float, float> radius)
        : center(center), radiusDist(radius.first, radius.second) {
    }

    void OnSphere::gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
        static std::uniform_real_distribution<float> distTheta(0.0f, 2 * static_cast<float>(std::numbers::pi));
        static std::uniform_real_distribution<float> distPhi(0.0f, static_cast<float>(std::numbers::pi));
        auto& gen = rng::get();

        for (std::size_t i = start_ix; i < end_ix; i++) {
            auto radius = radiusDist(gen);
            float theta = distTheta(gen);
            float phi = distPhi(gen);

            particles.pos[i] = {
                .x = center.x + radius * std::sin(phi) * std::cos(theta),
                .y = center.y + radius * std::cos(phi),
                .z = center.z + radius * std::sin(phi) * std::sin(theta),
            };
        }
    }
}

namespace particle_system::generators::velocity {
    Sphere::Sphere(float speed, std::optional<Vector3> hemisphere_vec) : speed(speed), hemisphere_vec(hemisphere_vec) {
    }

    void Sphere::gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
        static std::uniform_real_distribution<float> distTheta(0.0f, 2 * static_cast<float>(std::numbers::pi));
        static std::uniform_real_distribution<float> distPhi(0.0f, static_cast<float>(std::numbers::pi));
        auto& gen = rng::get();

        for (std::size_t i = start_ix; i < end_ix; i++) {
            float theta = distTheta(gen);
            float phi = distPhi(gen);

            particles.velocity[i] = {
                .x = speed * std::sin(phi) * std::cos(theta),
                .y = speed * std::cos(phi),
                .z = speed * std::sin(phi) * std::sin(theta),
            };

            if (hemisphere_vec && Vector3DotProduct(particles.velocity[i], *hemisphere_vec) < 0) {
                particles.velocity[i] = Vector3Negate(particles.velocity[i]);
            }
        }
    }

    ScaleRange::ScaleRange(float min, float max) : min(min), max(max), dist(min, max) {
    }

    void ScaleRange::gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
        auto& gen = rng::get();
        for (std::size_t i = start_ix; i < end_ix; i++) {
            particles.velocity[i] = Vector3Scale(particles.velocity[i], dist(gen));
        }
    }
}

namespace particle_system::generators::acceleration {
    Fixed::Fixed(Vector3 accel) : accel(accel) {
    }

    void Fixed::gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
        for (std::size_t i = start_ix; i < end_ix; i++) {
            particles.acceleration[i] = accel;
        }
    }

    Uniform::Uniform(float magnitude) : magnitude({magnitude, magnitude, magnitude}) {
    }
    Uniform::Uniform(Vector3 magnitude) : magnitude(magnitude) {
    }

    void Uniform::gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
        for (std::size_t i = start_ix; i < end_ix; i++) {
            auto normalized = Vector3Normalize(particles.velocity[i]);
            particles.acceleration[i].x = normalized.x * magnitude.x;
            particles.acceleration[i].y = normalized.y * magnitude.y;
            particles.acceleration[i].z = normalized.z * magnitude.z;
        }
    }
}

namespace particle_system::generators::color {
    Fixed::Fixed(Color color) : color(color) {
    }

    void Fixed::gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
        for (std::size_t i = start_ix; i < end_ix; i++) {
            particles.color[i] = color;
        }
    }
}

namespace particle_system::generators::lifetime {
    Range::Range(float min, float max) : min(min), max(max), dist(min, max) {
    }

    void Range::gen(Particles& particles, float dt, std::size_t start_ix, std::size_t end_ix) {
        auto& gen = rng::get();

        for (std::size_t i = start_ix; i < end_ix; i++) {
            particles.lifetime[i] = dist(gen);
        }
    }
}

namespace particle_system::updaters {
    void Lifetime::update(Particles& particles, float dt) {
        auto end_ix = particles.alive_count;

        for (std::size_t i = 0; i < end_ix; i++) {
            particles.lifetime[i] -= dt;

            if (particles.lifetime[i] <= 0.0f) {
                particles.kill(i);
                end_ix = particles.alive_count < particles.max_size ? particles.alive_count : particles.max_size;
            }
        }
    }

    void Position::update(Particles& particles, float dt) {
        for (std::size_t i = 0; i < particles.alive_count; i++) {
            particles.velocity[i] = Vector3Add(particles.velocity[i], Vector3Scale(particles.acceleration[i], dt));
            particles.pos[i] = Vector3Add(particles.pos[i], Vector3Scale(particles.velocity[i], dt));
        }
    }

    ColorByVelocity::ColorByVelocity(Color start, Color end, float min_threshold, float max_threshold)
        : start_col(start), end_col(end), min_threshold(min_threshold), max_threshold(max_threshold) {
    }

    void ColorByVelocity::update(Particles& particles, float dt) {
        for (std::size_t i = 0; i < particles.alive_count; i++) {
            auto speed = std::abs(Vector3Length(particles.velocity[i]));
            auto factor = (speed - min_threshold) / (max_threshold - min_threshold);

            if (factor < 0.0f) {
                particles.color[i] = ColorLerp(particles.color[i], start_col, std::abs(speed / min_threshold));
            } else {
                particles.color[i] = ColorLerp(start_col, end_col, factor);
            }
        }
    }
}

namespace particle_system::renderers {
    Point::Point(std::size_t max_size) : max_size(max_size) {
        setup_gl();
    }

    Point::~Point() {
        if (particle_circle.id != 0) UnloadTexture(particle_circle);
        rlUnloadVertexArray(vao_id);
        rlUnloadVertexBuffer(pos_vbo_id);
        rlUnloadVertexBuffer(col_vbo_id);
        rlUnloadVertexBuffer(col_vbo_id);
        if (shader.id != 0) UnloadShader(shader);
    }

    void Point::operator()(Particles& particles) {
        if (particles.alive_count == 0) return;

        BeginBlendMode(BLEND_ALPHA);

        std::memcpy(pos_vertex_data.get(), particles.pos.get(), pos_vertex_size * particles.alive_count);
        std::memcpy(size_vertex_data.get(), particles.size.get(), size_vertex_size * particles.alive_count);
        std::memcpy(col_vertex_data.get(), particles.color.get(), col_vertex_size * particles.alive_count);

        rlUpdateVertexBuffer(pos_vbo_id, pos_vertex_data.get(),
                             static_cast<int>(pos_vertex_size * particles.alive_count), 0);
        rlUpdateVertexBuffer(size_vbo_id, size_vertex_data.get(),
                             static_cast<int>(size_vertex_size * particles.alive_count), 0);
        rlUpdateVertexBuffer(col_vbo_id, col_vertex_data.get(),
                             static_cast<int>(col_vertex_size * particles.alive_count), 0);

        auto mvp = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
        SetShaderValueMatrix(shader, GetShaderLocation(shader, "mvp"), mvp);

        glUniform1i(glGetUniformLocation(shader.id, "circle"), 0);

        glDepthMask(GL_FALSE);
        glEnable(GL_PROGRAM_POINT_SIZE);
        glUseProgram(shader.id);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, particle_circle.id);

        rlEnableVertexArray(vao_id);
        glDrawArrays(GL_POINTS, 0, static_cast<int>(particles.alive_count));
        rlDisableVertexArray();

        glUseProgram(rlGetShaderIdDefault());
        rlDisableTexture();
        glDisable(GL_PROGRAM_POINT_SIZE);
        glDepthMask(GL_TRUE);

        EndBlendMode();
    }

    void Point::setup_gl() {
        vao_id = rlLoadVertexArray();
        pos_vbo_id = rlLoadVertexBuffer(NULL, static_cast<int>(pos_vertex_size * max_size), true);
        size_vbo_id = rlLoadVertexBuffer(NULL, static_cast<int>(size_vertex_size * max_size), true);
        col_vbo_id = rlLoadVertexBuffer(NULL, static_cast<int>(col_vertex_size * max_size), true);
        shader = LoadShader("./assets/particles.vs.glsl", "./assets/particles.fs.glsl");

        pos_vertex_data.reset(new uint8_t[max_size * pos_vertex_size]{});
        size_vertex_data.reset(new uint8_t[max_size * size_vertex_size]{});
        col_vertex_data.reset(new uint8_t[max_size * col_vertex_size]{});

        rlEnableVertexArray(vao_id);

        rlEnableVertexBuffer(pos_vbo_id);
        rlEnableVertexAttribute(0);
        rlSetVertexAttribute(0, 3, RL_FLOAT, false, pos_vertex_size, 0);

        rlEnableVertexBuffer(size_vbo_id);
        rlEnableVertexAttribute(1);
        rlSetVertexAttribute(1, 1, RL_FLOAT, false, size_vertex_size, 0);

        rlEnableVertexBuffer(col_vbo_id);
        rlEnableVertexAttribute(2);
        rlSetVertexAttribute(2, 4, RL_UNSIGNED_BYTE, true, col_vertex_size, 0);

        rlDisableVertexArray();
        rlDisableVertexBuffer();

        particle_circle = LoadTexture("./assets/particle_circle.png");
    }
}

namespace particle_system {
    System::System(std::size_t max_particles, renderers::Renderer&& renderer)
        : renderer(std::forward<renderers::Renderer>(renderer)), particles(max_particles) {
    }

    bool System::update(float dt) {
        bool can_emit = true;

        for (auto& e : emitters) {
            std::visit([&](auto&& arg) { can_emit = arg.emit(particles, dt) && can_emit; }, e);
        }

        for (auto& u : updaters) {
            std::visit(
                [&](auto&& arg) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, updaters::AnonUpdater>) {
                        arg(particles, dt);
                    } else {
                        arg.update(particles, dt);
                    }
                },
                u);
        }

        auto simulating = can_emit || particles.alive_count != 0;
        if (reset_on_done && !simulating) {
            reset(*reset_on_done);
            return true;
        }

        return simulating;
    }

    void System::draw() {
        std::visit([&](auto&& arg) { arg(particles); }, renderer);
    }

    void System::reset() {
        if (reset_on_done) {
            reset(*reset_on_done);
        } else {
            reset(particles.max_size);
        }
    }

    void System::reset(std::optional<std::size_t> max_emit) {
        particles.alive_count = 0;

        if (!max_emit) return;

        for (auto& emitter : emitters) {
            if (std::holds_alternative<emitters::CustomEmitter>(emitter)) {
                std::get<emitters::CustomEmitter>(emitter).max_emit = *max_emit;
            }
        }
    }

    void System::add_emitter(emitters::Emitter&& emitter) {
        emitters.emplace_back(std::forward<emitters::Emitter>(emitter));
    }

    void System::add_updater(updaters::Updater&& updater) {
        updaters.emplace_back(std::forward<updaters::Updater>(updater));
    }
}
