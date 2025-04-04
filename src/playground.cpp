#include "particle_system.hpp"

#include <raylib.h>

int main(void) {
    InitWindow(0, 0, "skibidi");

    Camera3D cam{};
    cam.position = {0.0f, 50.0f, 50.0f};
    cam.target = Vector3Zero();
    cam.up = {0.0f, 1.0f, 0.0f};
    cam.fovy = 90.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    particle_system::System system(2000000);

    particle_system::emitters::CustomEmitter<true> emitter;
    emitter.add_generator(particle_system::generators::pos::Fixed(Vector3Zero()));
    emitter.add_generator(particle_system::generators::velocity::Sphere(true, 1));
    emitter.add_generator(particle_system::generators::velocity::ScaleRange(5.0f, 15.0f));
    emitter.add_generator(particle_system::generators::acceleration::Uniform(Vector3{ .x = 10.0f, .y = 0.0f, .z = 10.0f }));
    emitter.add_generator(particle_system::generators::color::Fixed(WHITE));
    emitter.add_generator(particle_system::generators::lifetime::Range(0.5f, 1.5f));
    system.add_emitter(std::move(emitter));
    system.add_updater(particle_system::updaters::Position());
    system.add_updater(particle_system::updaters::Lifetime());
    system.add_updater(particle_system::updaters::ColorByVelocity(WHITE, BLUE, 17.0f, 25.0f));

    bool start = false;
    double prev_time = 0.;

    while (!WindowShouldClose()) {
        double current_time = GetTime();
        double delta_time = current_time - prev_time;
        prev_time = current_time;

        PollInputEvents();
        BeginDrawing();
            ClearBackground(WHITE);
            BeginMode3D(cam);
                DrawSphere(Vector3Zero(), 2.0f, BLUE);
                DrawGrid(1000, 5.0f);
                system.draw();
            EndMode3D();

            if (start) DrawText("STARTED", 10, 10, 20, BLACK);
        EndDrawing();
        SwapScreenBuffer();

        if (!start && IsKeyPressed(KEY_SPACE)) {
            start = true;
        }

        if (start) {
            system.update(delta_time);
        }
    }

    CloseWindow();

    return 0;
}
