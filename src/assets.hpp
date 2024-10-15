#pragma once

#include <array>
#include <utility>
#include <concepts>
#include <optional>

#include <raylib.h>

#include "spell.hpp"

// TODO: make this a config variable
#define MSAA 16

// ChatGipiti Made, works... probably
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

namespace assets {
    class Store;

    template<typename T>
    concept ToTexture2D = requires(Store s, T t) {
        { s[t] } -> std::same_as<Texture2D>;
    };

    enum GeneralId {
        EmptySpellSlot = 0,
        LockedSlot,
        GeneralIdSize,
    };

    enum RenderId {
        Target = 0,
        CircleUI,
        SpellBarUI,
        RenderIdSize,
    };

    class Store {
      public:
        Store(Vector2 screen) : texture_map({}), render_map({}) {
            add_textures();
            add_render_textures(screen);
        };
        Store(const Store&) = delete;

        ~Store() {
            for (auto& val : texture_map) {
                UnloadTexture(val);
            }

            for (auto& val : render_map) {
                UnloadRenderTexture(val);
            }
        };

        template<ToTexture2D Id>
        void draw_texture(Id texture_id, std::optional<Rectangle> dest) {
            auto tex = (*this)[texture_id];
            DrawTexturePro(tex,
                           (Rectangle){ 0.0f, 0.0f, (float)tex.width, (float)tex.height },
                           dest ? *dest : (Rectangle){ 0.0f, 0.0f, (float)tex.width, (float)tex.height },
                           Vector2Zero(), 0.0f, WHITE);
        }

        void draw_texture(RenderId render_id, bool resolved, std::optional<Rectangle> dest) {
            auto tex = (*this)[render_id, resolved].texture;
            DrawTexturePro(tex,
                           (Rectangle){ 0.0f, 0.0f, (float)tex.width, -(float)tex.height },
                           dest ? *dest : (Rectangle){ 0.0f, 0.0f, (float)tex.width, (float)tex.height },
                           Vector2Zero(), 0.0f, WHITE);
        }

        Texture2D operator [](GeneralId id) {
            return texture_map[id];
        }

        Texture2D operator [](Spell::Name name) {
            return texture_map[id_to_idx(name)];
        }

        Texture2D operator [](Rarity::Type id) {
            return texture_map[id_to_idx(id)];
        }

        RenderTexture2D operator [](RenderId id, bool resolved) {
            return index_render_map(id, resolved);
        }

        void update_target_size(Vector2 screen) {
            UnloadRenderTexture((*this)[Target, true]);
            UnloadRenderTexture((*this)[Target, false]);
            index_render_map(Target, false) = CreateRenderTextureMSAA(screen.x, screen.y, MSAA);
            index_render_map(Target, true) = LoadRenderTexture(screen.x, screen.y);
        }

      private:
        // order: GeneralIds, Names, Rarities
        std::array<Texture2D, GeneralIdSize + Spell::NameSize + static_cast<int>(Rarity::Size)> texture_map;
        std::array<RenderTexture2D, RenderIdSize*2> render_map;

        RenderTexture2D& index_render_map(RenderId id, bool resolved) {
            return render_map[id + (resolved ? static_cast<int>(RenderIdSize) : 0)];
        }

        int id_to_idx(GeneralId id) {
            return id;
        }

        int id_to_idx(Spell::Name name) {
            return name + GeneralIdSize;
        }

        int id_to_idx(Rarity::Type id) {
            return static_cast<int>(id) + GeneralIdSize + Spell::NameSize;
        }

        void add_textures() {
            Image img = LoadImage("./assets/spell-icons/empty-slot.png");
            texture_map[EmptySpellSlot] = LoadTextureFromImage(img);
            SetTextureFilter(texture_map[EmptySpellSlot], TEXTURE_FILTER_BILINEAR);
            UnloadImage(img);

            Image img2 = LoadImage("./assets/spell-icons/locked-slot.png");
            texture_map[LockedSlot] = LoadTextureFromImage(img2);
            SetTextureFilter(texture_map[LockedSlot], TEXTURE_FILTER_BILINEAR);
            UnloadImage(img2);

            for (int name_i = 0; name_i < Spell::NameSize; name_i++) {
                Image img = LoadImage((Spell::icon_path + "/" + Spell::icon_map[name_i]).c_str());
                texture_map[name_i + GeneralIdSize] = LoadTextureFromImage(img);
                SetTextureFilter(texture_map[name_i + GeneralIdSize], TEXTURE_FILTER_BILINEAR);
                UnloadImage(img);
            }

            for (int rarity_i = 0; rarity_i < Rarity::Size; rarity_i++) {
                Image img = LoadImage((Rarity::frame_path + "/" + Rarity::frames[rarity_i]).c_str());
                texture_map[rarity_i + Spell::NameSize + GeneralIdSize] = LoadTextureFromImage(img);
                SetTextureFilter(texture_map[rarity_i + Spell::NameSize + GeneralIdSize], TEXTURE_FILTER_BILINEAR);
                UnloadImage(img);
            }
        }

        void add_render_textures(Vector2 screen) {
            index_render_map(Target, false) = CreateRenderTextureMSAA(screen.x, screen.y, MSAA);
            index_render_map(Target, true) = LoadRenderTexture(screen.x, screen.y);
            index_render_map(CircleUI, false) = CreateRenderTextureMSAA(1023, 1023, MSAA); // odd, to have a center pixel
            index_render_map(CircleUI, true) = LoadRenderTexture(1023, 1023);
            index_render_map(SpellBarUI, false) = CreateRenderTextureMSAA(512, 128, MSAA);
            index_render_map(SpellBarUI, true) = LoadRenderTexture(512, 128);
        }
    };
}
