#include "utility.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <random>
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>

std::pair<int, Vector2> max_font_size(const Font& font, const Vector2& max_dims, std::string_view text) {
    int lo = 1;
    int hi = 1000; 
    int best_size = 0;
    Vector2 best_dims;

    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        float spacing = static_cast<float>(mid) / 10.0f;
        Vector2 dims = MeasureTextEx(font, text.data(), static_cast<float>(mid), spacing);

        if (dims.x <= max_dims.x && dims.y <= max_dims.y) {
            // Valid font size, try larger
            best_size = mid;
            best_dims = dims;
            lo = mid + 1;
        } else {
            // Too big, try smaller
            hi = mid - 1;
        }
    }

    return {best_size, best_dims};
}

// angle will be of range 0 - 360
// 0 degrees -> 12 o' clock
// 90 degrees -> 3 o' clock
// 180 degrees -> 6 o' clock
// 270 degrees -> 9 o' clock
float angle_from_point(const Vector2& point, const Vector2& origin) {
    return std::fmod(270 - std::atan2(origin.y - point.y, origin.x - point.x) * 180.0f / std::numbers::pi_v<float>,
                     360.0f);
}

Vector2 xz_component(const Vector3& vec) {
    return {vec.x, vec.z};
}

Vector2 mouse_xz_in_world(Ray mouse) {
    const float x = -mouse.position.y / mouse.direction.y;

    return (Vector2){mouse.position.x + x * mouse.direction.x, mouse.position.z + x * mouse.direction.z};
}

float wrap(float value, float modulus) {
    return value - modulus * std::floor(value / modulus);
}

std::string format_to_time(double time) {
    uint64_t total_seconds = static_cast<uint64_t>(time);
    uint64_t hours = total_seconds / 3600;
    uint64_t minutes = (total_seconds % 3600) / 60;
    uint64_t seconds = total_seconds % 60;

    return std::format("{:02}:{:02}:{:02}", hours, minutes, seconds);
}

Vector4 spellbook_and_tile_dims(Vector2 screen, Vector2 spellbook_dims, Vector2 tile_dims) {
    auto spellbook_width = screen.x * 0.3f;
    auto spellbook_height = height_from_ratio(spellbook_dims, spellbook_width);
    if (spellbook_height > screen.y * 0.95f) {
        spellbook_height = screen.y * 0.95f;
        spellbook_width = width_from_ratio(spellbook_dims, spellbook_height);
    }

    return Vector4{
        spellbook_width,
        spellbook_height,
        spellbook_width * 0.75f,
        height_from_ratio(tile_dims, spellbook_width * 0.75f),
    };
}

Vector3 wrap_lerp(Vector3 a, Vector3 b, float t) {
    float half_width  = ARENA_WIDTH  / 2.0f;
    float half_height = ARENA_HEIGHT / 2.0f;

    float dx = b.x - a.x;
    if (dx >  half_width)  b.x -= ARENA_WIDTH;
    if (dx < -half_width)  b.x += ARENA_WIDTH;
    float x = a.x + (b.x - a.x) * t;
    if (x < -half_width) x += ARENA_WIDTH;
    if (x >  half_width) x -= ARENA_WIDTH;

    float y = a.y + (b.y - a.y) * t;

    float dz = b.z - a.z;
    if (dz >  half_height) b.z -= ARENA_HEIGHT;
    if (dz < -half_height) b.z += ARENA_HEIGHT;
    float z = a.z + (b.z - a.z) * t;
    if (z < -half_height) z += ARENA_HEIGHT;
    if (z >  half_height) z -= ARENA_HEIGHT;

    return Vector3{ x, y, z };
}

Mesh gen_mesh_quad(Vector3 p1, Vector3 p2, Vector3 p3, Vector3 p4) {
    std::vector ps = {p1, p2, p3, p4};
    Mesh mesh{};
    mesh.vertexCount = 4;
    mesh.triangleCount = 2;

    auto normal = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(p2, p1), Vector3Subtract(p3, p1)));

    mesh.vertices = static_cast<float*>(RL_MALLOC(static_cast<unsigned long>(mesh.vertexCount)*sizeof(Vector3)));
    mesh.normals = static_cast<float*>(RL_MALLOC(static_cast<unsigned long>(mesh.vertexCount)*sizeof(Vector3)));
    for (std::size_t i = 0; i < ps.size(); i++) {
        reinterpret_cast<Vector3*>(mesh.vertices)[i] = ps[i];
        reinterpret_cast<Vector3*>(mesh.normals)[i] = normal;
    }

    mesh.texcoords = static_cast<float*>(RL_MALLOC(static_cast<unsigned long>(mesh.vertexCount)*sizeof(Vector3)));
    auto texcoords = reinterpret_cast<Vector2*>(mesh.texcoords);
    texcoords[3] = {0, 1};
    texcoords[2] = {1, 1};
    texcoords[1] = {1, 0};
    texcoords[0] = {0, 0};

    mesh.indices = static_cast<unsigned short*>(RL_MALLOC(static_cast<unsigned long>(mesh.triangleCount*3)*sizeof(unsigned short)));
    auto indices = reinterpret_cast<unsigned short*>(mesh.indices);
    indices[0] = 2;
    indices[1] = 1;
    indices[2] = 0;

    indices[3] = 0;
    indices[4] = 3;
    indices[5] = 2;

    UploadMesh(&mesh, false);
    return mesh;
}

void DrawBillboardCustom(Camera camera, Texture2D texture, Rectangle source, Vector3 position, Vector3 up, Vector2 size, Vector2 origin, Vector3 rotations, Color tint) {
    // Compute the up vector and the right vector
    Matrix matView = MatrixLookAt(camera.position, camera.target, camera.up);
    Vector3 right = { matView.m0, matView.m4, matView.m8 };
    right = Vector3Scale(right, size.x);
    up = Vector3Scale(up, size.y);

    // Flip the content of the billboard while maintaining the counterclockwise edge rendering order
    if (size.x < 0.0f)
    {
        source.x += size.x;
        source.width *= -1.0;
        right = Vector3Negate(right);
        origin.x *= -1.0f;
    }
    if (size.y < 0.0f)
    {
        source.y += size.y;
        source.height *= -1.0;
        up = Vector3Negate(up);
        origin.y *= -1.0f;
    }

    // Draw the texture region described by source on the following rectangle in 3D space:
    //
    //                size.x          <--.
    //  3 ^---------------------------+ 2 \ rotation
    //    |                           |   /
    //    |                           |
    //    |   origin.x   position     |
    // up |..............             | size.y
    //    |             .             |
    //    |             . origin.y    |
    //    |             .             |
    //  0 +---------------------------> 1
    //                right
    Vector3 forward;

    Vector3 origin3D = Vector3Add(Vector3Scale(Vector3Normalize(right), origin.x), Vector3Scale(Vector3Normalize(up), origin.y));

    Vector3 points[4];
    points[0] = Vector3Zero();
    points[1] = right;
    points[2] = Vector3Add(up, right);
    points[3] = up;

    for (int i = 0; i < 4; i++)
    {
        points[i] = Vector3Subtract(points[i], origin3D);
        // if (angle != 0.0f) points[i] = Vector3RotateByAxisAngle(points[i], rotation_axis, angle * DEG2RAD);
        if (rotations.x != 0.0f) points[i] = Vector3RotateByAxisAngle(points[i], Vector3{1.0f, 0.0f, 0.0f}, rotations.x * DEG2RAD);
        if (rotations.y != 0.0f) points[i] = Vector3RotateByAxisAngle(points[i], Vector3{0.0f, 1.0f, 0.0f}, rotations.y * DEG2RAD);
        if (rotations.z != 0.0f) points[i] = Vector3RotateByAxisAngle(points[i], Vector3{0.0f, 0.0f, 1.0f}, rotations.z * DEG2RAD);
        points[i] = Vector3Add(points[i], position);
    }

    Vector2 texcoords[4];
    texcoords[0] = (Vector2) { (float)source.x/texture.width, (float)(source.y + source.height)/texture.height };
    texcoords[1] = (Vector2) { (float)(source.x + source.width)/texture.width, (float)(source.y + source.height)/texture.height };
    texcoords[2] = (Vector2) { (float)(source.x + source.width)/texture.width, (float)source.y/texture.height };
    texcoords[3] = (Vector2) { (float)source.x/texture.width, (float)source.y/texture.height };

    rlSetTexture(texture.id);
    rlBegin(RL_QUADS);

        rlColor4ub(tint.r, tint.g, tint.b, tint.a);
        for (int i = 0; i < 4; i++)
        {
            rlTexCoord2f(texcoords[i].x, texcoords[i].y);
            rlVertex3f(points[i].x, points[i].y, points[i].z);
        }

    rlEnd();
    rlSetTexture(0);
}

void arena::loop_around(float& x, float& y) {
    if (x > ARENA_WIDTH / 2.0f) x -= ARENA_WIDTH;
    if (y > ARENA_HEIGHT / 2.0f) y -= ARENA_HEIGHT;
    if (x < -ARENA_HEIGHT / 2.0f) x += ARENA_WIDTH;
    if (y < -ARENA_HEIGHT / 2.0f) y += ARENA_HEIGHT;
}

std::mt19937 rng_gen(static_cast<unsigned long>(std::chrono::steady_clock::now().time_since_epoch().count()));

std::mt19937& rng::get() {
    return rng_gen;
}
