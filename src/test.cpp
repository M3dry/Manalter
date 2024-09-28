#include <cstddef>

#include "raylib.h"
#include "glad.h"  // For OpenGL functions

// RenderTexture2D CreateRenderTextureMSAA(int width, int height, int samples) {
//     RenderTexture2D target = { 0 };

//     // Create an empty MSAA framebuffer
//     glGenFramebuffers(1, &target.id);
//     glBindFramebuffer(GL_FRAMEBUFFER, target.id);

//     // Create the multisampled color renderbuffer (color attachment)
//     glGenRenderbuffers(1, &target.texture.id);
//     glBindRenderbuffer(GL_RENDERBUFFER, target.texture.id);
//     glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, width, height); // MSAA color buffer
//     glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, target.texture.id);

//     // Create and attach multisample depth renderbuffer
//     glGenRenderbuffers(1, &target.depth.id);
//     glBindRenderbuffer(GL_RENDERBUFFER, target.depth.id);
//     glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT24, width, height); // MSAA depth buffer
//     glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, target.depth.id);

//     // Check if the framebuffer is complete
//     if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) 
//     {
//         return target;
//     }

//     // Now, create a standard framebuffer to which we'll resolve (blit) the MSAA buffer
//     glGenFramebuffers(1, &target.texture.id);
//     glBindFramebuffer(GL_FRAMEBUFFER, target.texture.id);

//     // Create a color texture (non-MSAA) for the resolved texture
//     glGenTextures(1, &target.texture.id);
//     glBindTexture(GL_TEXTURE_2D, target.texture.id);
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
//     
//     // Attach texture to the framebuffer
//     glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.texture.id, 0);

//     // Check the standard framebuffer completeness
//     if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) 
//     {
//         return target;
//     }

//     glBindFramebuffer(GL_FRAMEBUFFER, 0);  // Unbind the framebuffer

//     return target;
// }

// Helper function to create a RenderTexture2D with MSAA
RenderTexture2D CreateRenderTextureMSAA(int width, int height, int samples) {
    RenderTexture2D target = { 0 };

    // Step 1: Create a Framebuffer Object (FBO)
    glGenFramebuffers(1, &target.id);
    glBindFramebuffer(GL_FRAMEBUFFER, target.id);

    // Step 2: Create a Multisample Renderbuffer for color
    GLuint colorBufferMSAA;
    glGenRenderbuffers(1, &colorBufferMSAA);
    glBindRenderbuffer(GL_RENDERBUFFER, colorBufferMSAA);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorBufferMSAA);

    // Step 3: Create a Multisample Renderbuffer for depth (optional)
    GLuint depthBufferMSAA;
    glGenRenderbuffers(1, &depthBufferMSAA);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBufferMSAA);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBufferMSAA);

    // Step 4: Check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        TraceLog(LOG_WARNING, "Framebuffer is not complete!");
    }

    // Create a standard texture to blit the multisampled FBO to
    glGenTextures(1, &target.texture.id);
    glBindTexture(GL_TEXTURE_2D, target.texture.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Create the depth texture (standard depth format, not compressed)
    glGenTextures(1, &target.depth.id);
    glBindTexture(GL_TEXTURE_2D, target.depth.id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

    // Create and assign the Texture2D object
    target.texture.width = width;
    target.texture.height = height;
    target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;  // RGBA format
    target.texture.mipmaps = 1;

    // Create and assign the Depth texture
    target.depth.width = width;
    target.depth.height = height;
    target.depth.format = PIXELFORMAT_UNCOMPRESSED_R32;  // Correct depth format
    target.depth.mipmaps = 1;

    // Unbind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return target;
}

void EndTextureModeMSAA(RenderTexture2D target, RenderTexture2D resolveTarget) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, target.id);  // Bind the MSAA framebuffer (read source)
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveTarget.id); // Bind the non-MSAA framebuffer (write target)

    // Blit the multisampled framebuffer to the non-multisampled texture
    glBlitFramebuffer(0, 0, resolveTarget.texture.width, resolveTarget.texture.height, 
                      0, 0, resolveTarget.texture.width, resolveTarget.texture.height, 
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);

    // Unbind framebuffer after resolving
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int main() {
    const int screenWidth = 1920;
    const int screenHeight = 1200;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - MSAA Render Texture");

    int samples = 4;  // Define MSAA sample count
    RenderTexture2D renderTexture = CreateRenderTextureMSAA(screenWidth, screenHeight, samples);
    RenderTexture2D resolved = LoadRenderTexture(screenWidth, screenHeight);
    RenderTexture2D fxaa_tex = LoadRenderTexture(screenWidth, screenHeight);
    Shader fxaa_shader = LoadShader(0, "./assets/fxaa.glsl");

    Vector2 screen = { screenWidth, screenHeight };
    SetShaderValue(fxaa_shader, GetShaderLocation(fxaa_shader, "resolution"), &screen, SHADER_UNIFORM_VEC2);
    SetShaderValueTexture(fxaa_shader, GetShaderLocation(fxaa_shader, "texture0"), fxaa_tex.texture); 

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // Use BeginTextureMode to render to the texture
        BeginTextureMode(renderTexture);
            DrawText("SKIBIDI", 10, 10, 20, BLACK);
            DrawCircle(340, 700, 300.0f, BLACK);
        EndTextureMode();
        EndTextureModeMSAA(renderTexture, resolved);

        BeginTextureMode(fxaa_tex);
                ClearBackground(RAYWHITE);
                DrawCircle(960, 700, 300.0f, BLACK);
        EndTextureMode();

        // Now we can render the texture to the screen
        BeginDrawing();
            ClearBackground(RAYWHITE);

            // Draw the texture to the main framebuffer (screen)
            BeginShaderMode(fxaa_shader);
                DrawTexturePro(fxaa_tex.texture,
                               (Rectangle){ 1.0f, 0.0f, (float)fxaa_tex.texture.width, -(float)fxaa_tex.texture.height },
                               (Rectangle){ 0.0f, 0.0f, screenWidth, screenHeight },
                               (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
            EndShaderMode();
            DrawTexturePro(resolved.texture,
                           (Rectangle){ 1.0f, 0.0f, (float)resolved.texture.width, -(float)resolved.texture.height },
                           (Rectangle){ 0.0f, 0.0f, screenWidth, screenHeight },
                           (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
            DrawCircle(1580, 700, 300.0f, BLACK);
        EndDrawing();
    }

    UnloadRenderTexture(renderTexture);  // Unload the custom render texture
    UnloadRenderTexture(resolved);
    UnloadRenderTexture(fxaa_tex);
    UnloadShader(fxaa_shader);
    CloseWindow();  // Close window and OpenGL context

    return 0;
}
