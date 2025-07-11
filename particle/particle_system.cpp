#include <cstdint>
#include <julip/julip.hpp>
#include <particle/particle.hpp>
#include <raylib.h>

#include <optional>

struct SystemOpts {
    float emit_rate;
    std::size_t max_emit;

    std::size_t max_particles;
    std::size_t reset_on_done;

    Vector3 origin;
};
JULIA_STRUCT_NAME(SystemOpts, SystemOpts);
JULIA_STRUCT_NAME(particle_system::Particles, CParticles);

struct System {
    SystemOpts opts;
    julip::value generator;
    julip::value updater;

    particle_system::System construct() {
        particle_system::System system(opts.max_particles,
                                       particle_system::renderers::Point(opts.max_particles, opts.origin));
        system.reset_on_done = opts.reset_on_done == 0 ? std::nullopt : std::make_optional(opts.reset_on_done);
        particle_system::emitters::CustomEmitter emitter(opts.emit_rate, opts.max_emit);

        emitter.add_generator(
            [gen = this->generator](auto& p, auto dt, auto start_ix, auto end_ix) { gen(&p, dt, start_ix + 1, end_ix); });
        system.add_emitter(std::move(emitter));
        system.add_updater([upd = this->updater](auto& p, auto dt) { upd(&p, dt); });

        return system;
    }
};

int main(int argc, char** argv) {
    julip::init();

    std::vector<System> systems;

    julip::Main.eval(R"(
        struct SystemOpts
            emit_rate::Float32
            max_emit::UInt

            max_particles::UInt
            # 0 = nullopt
            reset_on_done::UInt

            origin::NTuple{3, Float32}
        end

        struct CParticles
            pos::Ptr{NTuple{3, Cfloat}}
            velocity::Ptr{NTuple{3, Cfloat}}
            acceleration::Ptr{NTuple{3, Cfloat}}
            color::Ptr{NTuple{4, Cuchar}}
            alive::Ptr{Cuchar}
            lifetime::Ptr{Cfloat}
            size::Ptr{Cfloat}

            max_size::Csize_t
            alive_count::Csize_t
        end
    )");

    julip::Main.set_function("new_system",
                             [&systems](julip::value gen, julip::value updater, SystemOpts opts) -> uint64_t {
                                 systems.emplace_back(opts, std::move(gen), std::move(updater));
                                 return systems.size() - 1;
                             });

    julip::Main.set_function("_kill!", [](void* p, size_t ix) {
        ((particle_system::Particles*)p)->kill(ix);
        return ((particle_system::Particles*)p)->alive_count;
    });
    julip::Main.set_function("_wake!", [](void* p, size_t ix) {
        ((particle_system::Particles*)p)->wake(ix);
        return ((particle_system::Particles*)p)->alive_count;
    });
    julip::Main.set_function("_swap!",
                             [](void* p, size_t a, size_t b) { ((particle_system::Particles*)p)->swap(a, b); });

    uint32_t ticks = 60;
    julip::Main.set_function("set_ticks!", [&](uint32_t new_ticks) {
        ticks = new_ticks;
    });

    julip::Main.include(argv[1]);

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
    float prev_time = 0.;
    float accum_time = 0.;
    while (!WindowShouldClose()) {
        float current_time = static_cast<float>(GetTime());
        float delta_time = current_time - prev_time;
        prev_time = current_time;

        accum_time += delta_time;
        while (accum_time >= 1.f/static_cast<float>(ticks)) {
            if (start) {
                for (auto& particle_system : particle_systems) {
                    if (!particle_system.update(accum_time)) {
                        start = false;
                        particle_system.reset();
                    }
                }
            }

            accum_time = 0.;
        }

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
        DrawText(std::format("TICKS: {}", ticks).c_str(), 10, 30, 20, WHITE);
        EndDrawing();
        SwapScreenBuffer();

        if (!start && IsKeyPressed(KEY_SPACE)) {
            start = true;
        }

        if (IsKeyPressed(KEY_K)) ticks++;
        if (IsKeyPressed(KEY_J)) ticks--;

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

    particle_systems.clear();

    CloseWindow();
    julip::destruct();
    return 0;
}
