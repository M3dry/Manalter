#include "effects.hpp"
#include "particle.hpp"

#include <limits>
#include <raylib.h>
#include <raymath.h>

#include <print>

#include <sol/sol.hpp>
#include <stdexcept>
#include <string>

struct SystemData {
    std::vector<sol::function> generators;
    std::vector<sol::function> updaters;

    float emit_rate;
    std::size_t max_emit;

    std::size_t max_particles;
    std::optional<std::size_t> reset_on_done;

    SystemData(std::size_t max_particles, std::optional<std::size_t> reset_on_done, float emit_rate,
               std::size_t max_emit)
        : emit_rate(emit_rate), max_emit(max_emit), max_particles(max_particles), reset_on_done(reset_on_done) {};

    void print() const {
        std::println("generator count: {}", generators.size());
        std::println("updaters count: {}", updaters.size());
        std::println("emit rate: {}", emit_rate);
        std::println("max emit: {}", max_emit);
        std::println("max_particles: {}", max_particles);
        std::println("reset_on_done: {}", reset_on_done ? std::to_string(*reset_on_done) : "nullopt");
    }

    particle_system::System construct_system() const {
        particle_system::System system(max_particles, particle_system::renderers::Point(max_particles));
        system.reset_on_done = reset_on_done;

        particle_system::emitters::CustomEmitter emitter(emit_rate, max_emit);
        for (const auto& generator : generators) {
            emitter.add_generator(generator);
        }

        system.add_emitter(std::move(emitter));

        for (const auto& updater : updaters) {
            system.add_updater(updater);
        }

        return system;
    }
};

template <typename T> struct ArrayProxy {
    T* data;
    std::size_t size;

    T& operator[](std::size_t i) {
        if (i >= size) throw std::out_of_range("Index out of range");

        return data[i];
    }

    std::size_t length() const {
        return size;
    }

    static void bind(sol::state& lua, const char* name) {
        lua.new_usertype<ArrayProxy<T>>(
            name, sol::meta_function::index, [](ArrayProxy<T>& arr, std::size_t i) -> T& { return arr[i]; },
            sol::meta_function::new_index, [](ArrayProxy<T>& arr, std::size_t i, const T& value) { arr[i] = value; },
            sol::meta_function::length, &ArrayProxy<T>::length);
    }
};

int main(int argc, char** argv) {
    sol::state lua;
    std::vector<SystemData> systems;

    lua.open_libraries(sol::lib::base, sol::lib::math);

    lua.new_usertype<Vector3>("Vector3", sol::constructors<Vector3(), Vector3(float, float, float)>(), "x", &Vector3::x,
                              "y", &Vector3::y, "z", &Vector3::z);
    lua.new_usertype<Color>("Color", "r", &Color::r, "g", &Color::g, "b", &Color::b, "a", &Color::a);
    ArrayProxy<Vector3>::bind(lua, "ArrayProxyVector3");
    ArrayProxy<Color>::bind(lua, "ArrayProxyColor");
    ArrayProxy<float>::bind(lua, "ArrayProxyfloat");
    ArrayProxy<bool>::bind(lua, "ArrayProxybool");

    {
#define PROPERTY(x, t) sol::property([](Particles& p) { return ArrayProxy<t>{p.x.get(), p.max_size}; })
        using particle_system::Particles;

        lua.new_usertype<Particles>("Particles", "max_size", &Particles::max_size, "alive_count",
                                    &Particles::alive_count, "kill", &Particles::kill, "wake", &Particles::wake, "swap",
                                    &Particles::swap, "pos", PROPERTY(pos, Vector3), "velocity",
                                    PROPERTY(velocity, Vector3), "acceleration", PROPERTY(acceleration, Vector3),
                                    "color", PROPERTY(color, Color), "alive", PROPERTY(alive, bool), "lifetime",
                                    PROPERTY(lifetime, float), "size", PROPERTY(size, float));
#undef PROPERTY
    }

    lua["new_system"] = [&systems](std::size_t max_particles, std::optional<std::size_t> reset_on_done,
                                   std::optional<float> emit_rate, std::optional<std::size_t> max_emit) -> std::size_t {
        systems.emplace_back(max_particles, reset_on_done,
                             emit_rate.value_or(static_cast<float>(std::numeric_limits<std::size_t>::max())),
                             max_emit.value_or(std::numeric_limits<std::size_t>::max()));

        return systems.size() - 1;
    };

    lua["add_generator"] = [&systems](sol::function generator, std::size_t system_id) {
        if (systems.size() <= system_id) {
            std::println("ignoring generator, system_id `{}` is invalid", system_id);
            return;
        }

        systems[system_id].generators.emplace_back(std::move(generator));
    };
    lua["add_updater"] = [&systems](sol::function updater, std::size_t system_id) {
        if (systems.size() <= system_id) {
            std::println("ignoring updater, system_id `{}` is invalid", system_id);
            return;
        }

        systems[system_id].updaters.emplace_back(std::move(updater));
    };

    assert(argc > 1);
    lua.script_file(argv[1]);

    for (const auto& system : systems) {
        std::println("");
        system.print();
    }

    InitWindow(0, 0, "Particle system");
    SetWindowState(FLAG_FULLSCREEN_MODE);

    std::vector<particle_system::System> particle_systems;
    for (const auto& system : systems) {
        particle_systems.emplace_back(system.construct_system());
    }

    Camera3D cam{};
    cam.position = {0.0f, 10.0f, 30.0f};
    cam.target = Vector3Zero();
    cam.up = {0.0f, 1.0f, 0.0f};
    cam.fovy = 90.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    bool start = false;
    double prev_time = 0.;
    while (!WindowShouldClose()) {
        double current_time = GetTime();
        double delta_time = current_time - prev_time;
        prev_time = current_time;

        PollInputEvents();
        BeginDrawing();
        ClearBackground(DARKGRAY);
        BeginMode3D(cam);
        DrawSphere(Vector3Zero(), 0.5f, RED);
        DrawPlane(Vector3Zero(), {15000.0f, 15000.0f}, DARKGRAY);
        DrawCircle3D(Vector3Zero(), 10.0f, {1.0f, 0.0f, 0.0f}, 90.0f, WHITE);
        DrawCircle3D(Vector3Zero(), 5.0f, {1.0f, 0.0f, 0.0f}, 90.0f, WHITE);
        EndMode3D();

        BeginMode3D(cam);

        for (auto& particle_system : particle_systems) {
            particle_system.draw();
        }

        EndMode3D();

        if (start) DrawText("STARTED", 10, 10, 20, WHITE);
        EndDrawing();
        SwapScreenBuffer();

        if (!start && IsKeyPressed(KEY_SPACE)) {
            start = true;
        }

        if (start) {
            for (auto& particle_system : particle_systems) {
                if (!particle_system.update(static_cast<float>(delta_time))) {
                    start = false;
                    particle_system.reset();
                }
            }
        }
    }

    CloseWindow();

    return 0;
}
