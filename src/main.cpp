#include <print>
#include <cassert>

#include <raylib.h>
#include <raymath.h>

#define TICKS 120

using Player = struct Player {
    Vector3 position;
    float angle;
    Model model;
    Camera3D camera;
};

Vector2 mouse_xz_in_world(Ray mouse) {
    const float x = -mouse.position.y/mouse.direction.y;

    assert(mouse.position.y + x*mouse.direction.y == 0);

    return (Vector2){ mouse.position.x + x*mouse.direction.x, mouse.position.z + x*mouse.direction.z };
}

void update(Player& player, Vector2& mouse_pos, Vector2 screen) {
    screen = (Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() };

    static const int keys[] = { KEY_A, KEY_S, KEY_D, KEY_W };
    static const Vector2 movements[] = { { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 } };
    static const float angles[] = { 0.0f, 90.0f, 180.0f, 270.0f };

    Vector2 movement = { 0, 0 };
    Vector2 angle = { 0.0f, 0.0f };

    for (int k = 0; k < 4; k++) {
        if (IsKeyDown(keys[k])) {
            movement = Vector2Add(movement, movements[k]);
            angle.x += (k == 3 && angle.x == 0) ? -90.0f : angles[k];
            angle.y++;
        }
    }
    float length = Vector2Length(movement);
    if (length != 1 && length != 0) movement = Vector2Divide(movement, { length, length });

    if (movement.x != 0 || movement.y != 0) {
        auto x = movement.x/100; 
        auto z = movement.y/100;
        player.position.x += x;
        player.position.z += z;

        player.camera.position.x += x;
        player.camera.position.z += z;
        player.camera.target = player.position;

        player.angle = angle.x/angle.y;
    }

    mouse_pos = mouse_xz_in_world(GetMouseRay(GetMousePosition(), player.camera));
}

int main() {
    InitWindow(0, 0, "Aetas Magus");
    SetWindowState(FLAG_FULLSCREEN_MODE);
    SetExitKey(KEY_NULL);

    Vector2 screen = (Vector2){ (float)GetScreenWidth(), (float)GetScreenHeight() };

    Player player = {
        .position = { 0.0f, 10.0f, 0.0f },
        .angle = 0.0f,
        .model = LoadModel("./assets/player/player.obj"),
        .camera = {0},
    };


    player.camera.position = (Vector3){ 30.0f, 70.0f, 0.0f };
    player.camera.target = player.position;
    player.camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    player.camera.fovy = 90.0f;
    player.camera.projection = CAMERA_PERSPECTIVE;

    Vector2 mouse_pos = (Vector2){0.0f, 0.0f};

    bool tick = true;
    double mili_accum = 0;

    while (!WindowShouldClose()) {
        double time = GetTime();
        if (tick) {
            tick = false;
            PollInputEvents();
            update(player, mouse_pos, screen);
        }

        BeginDrawing();
            ClearBackground(WHITE);
            BeginMode3D(player.camera);
                DrawGrid(1000, 5.0f);
                DrawModelEx(player.model, player.position, (Vector3){ 0.0f, 1.0f, 0.0f }, player.angle, (Vector3){ 0.2f, 0.2f, 0.2f }, ORANGE);
                DrawCylinder((Vector3){ mouse_pos.x, 0.0f, mouse_pos.y }, 5.0f, 5.0f, 1.0f, 128, YELLOW);
            EndMode3D();
        EndDrawing();
        SwapScreenBuffer();

        mili_accum += GetTime() - time;
        if (mili_accum > TICKS/1000) {
            mili_accum = 0;
            tick = true;
        }
    }

    UnloadModel(player.model);

    CloseWindow();

    return 0;
}
