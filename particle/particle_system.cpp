#include "particle/particle.hpp"
#include <cstddef>
#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <julia.h>
#include <print>
#include <raylib.h>
#include <sstream>
#include <string>
#include <vector>

JULIA_DEFINE_FAST_TLS

extern "C" struct CSystemOpts {
    float emit_rate;
    size_t max_emit;

    size_t max_particles;
    // 0 - nullopt
    size_t reset_on_done;

    float origin[3];
};

struct SystemOpts {
    float emit_rate;
    std::size_t max_emit;

    std::size_t max_particles;
    std::optional<std::size_t> reset_on_done;

    Vector3 origin;

    SystemOpts(CSystemOpts opts)
        : emit_rate(opts.emit_rate), max_emit(opts.max_emit), max_particles(opts.max_particles),
          reset_on_done(opts.reset_on_done == 0 ? std::nullopt : std::make_optional(opts.reset_on_done)),
          origin(opts.origin[0], opts.origin[1], opts.origin[2]) {};

    void print() const {
        std::println("emit rate: {}", emit_rate);
        std::println("max emit: {}", max_emit);
        std::println("max particles: {}", max_particles);
        std::println("reset on done: {}", reset_on_done ? std::to_string(*reset_on_done) : "nullopt");
        std::println("origin: [{}; {}; {}]", origin.x, origin.y, origin.z);
    }
};

struct System {
    using Generator = void(void*, particle_system::Particles*, float, size_t, size_t);
    using Updater = void(void*, particle_system::Particles*, float);

    SystemOpts opts;
    Generator* generator;
    Updater* updater;

    void* thunk;

    particle_system::System construct() {
        particle_system::System system(opts.max_particles,
                                       particle_system::renderers::Point(opts.max_particles, opts.origin));
        system.reset_on_done = opts.reset_on_done;
        particle_system::emitters::CustomEmitter emitter(opts.emit_rate, opts.max_emit);

        emitter.add_generator(
            [&](auto& p, auto dt, auto start_ix, auto end_ix) { generator(thunk, &p, dt, start_ix, end_ix); });
        system.add_emitter(std::move(emitter));
        system.add_updater([&](auto& p, auto dt) { updater(thunk, &p, dt); });

        return system;
    }
};

std::vector<System> systems;
// particle_system::Particles particles(300000);

extern "C" {
size_t ffi__new_system(void* thunk, System::Generator* generator, System::Updater* updater, CSystemOpts opts) {
    systems.emplace_back(opts, generator, updater, thunk);

    return systems.size() - 1;
}

void ffi__kill_particle(particle_system::Particles* p, size_t ix) {
    p->kill(ix);
}

void ffi__wake_particle(particle_system::Particles* p, size_t ix) {
    p->wake(ix);
}

void ffi__swap_particles(particle_system::Particles* p, size_t a, size_t b) {
    p->swap(a, b);
}

void ffi__set_alive(particle_system::Particles* p, size_t val) {
    p->alive_count = val;
}

// particle_system::Particles* ffi__get_particles() {
//     return &particles;
// }
}

int main(int argc, char* argv[]) {
    // magic to make julia -> FFI work
    void* handle = dlopen(NULL, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        fprintf(stderr, "dlopen(NULL) failed: %s\n", dlerror());
        exit(1);
    }

    const char* script_path = argv[1];
    std::ifstream ifs(script_path);
    std::ostringstream ss;
    ss << ifs.rdbuf();
    std::string script = ss.str();

    jl_init();
    jl_eval_string(script.c_str());

    jl_function_t* f = jl_get_function(jl_main_module, "__main");
    jl_call0(f);
    if (jl_exception_occurred()) {
        printf("%s \n", jl_typeof_str(jl_exception_occurred()));
    }

    InitWindow(0, 0, "Particle system");
    SetWindowState(FLAG_FULLSCREEN_MODE);
    DisableCursor();

    std::vector<particle_system::System> particle_systems;
    for (auto& system : systems) {
        particle_systems.emplace_back(system.construct());
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
        ClearBackground(GRAY); // ClearBackground(state.background_color);
        BeginMode3D(cam);
        DrawSphere(Vector3Zero(), 0.5f, RED);
        DrawPlane(Vector3Zero(), {15000.0f, 15000.0f},
                  DARKGRAY); // DrawPlane(Vector3Zero(), {15000.0f, 15000.0f}, state.plane_color);
        DrawCircle3D(Vector3{0.0f, 0.5f, 0.0f}, 10.0f, {1.0f, 0.0f, 0.0f}, 90.0f, WHITE);
        DrawCircle3D(Vector3{0.0f, 0.5f, 0.0f}, 5.0f, {1.0f, 0.0f, 0.0f}, 90.0f, WHITE);
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

        // if (IsKeyPressed(KEY_R)) {
        //     try {
        //         LuaState new_state(argv[1]);
        //
        //         state.~LuaState();
        //         new (&state) LuaState(std::move(new_state));
        //         state.construct_particle_systems();
        //     } catch (std::exception e) {
        //         std::println("Your lua is fucked, I have it printed out");
        //     }
        // }

        // if (IsKeyPressed(KEY_T)) {
        //     state.tracker();
        // }

        UpdateCameraPro(&cam,
                        (Vector3){
                            (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) * 0.1f - // Move forward-backward
                                (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) * 0.1f,
                            (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) * 0.1f - // Move right-left
                                (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) * 0.1f,
                            0.0f // Move up-down
                        },
                        (Vector3){
                            GetMouseDelta().x * 0.05f, // Rotation: yaw
                            GetMouseDelta().y * 0.05f, // Rotation: pitch
                            0.0f                       // Rotation: roll
                        },
                        GetMouseWheelMove() * 2.0f); // Move to target (zoom)
    }

    CloseWindow();
    jl_atexit_hook(0);
    return 0;
}
