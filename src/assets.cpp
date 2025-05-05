#include "assets.hpp"
#include "raylib.h"
#include "spell.hpp"

#ifndef PLATFORM_WEB
RenderTexture2D CreateRenderTextureMSAA(int width, int height, int samples) {
    return LoadRenderTexture(width, height);

    RenderTexture2D target{};

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
    target.texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8; // RGBA format
    target.texture.mipmaps = 1;

    // Create and assign the Depth texture
    target.depth.width = width;
    target.depth.height = height;
    target.depth.format = PIXELFORMAT_UNCOMPRESSED_R32; // Correct depth format
    target.depth.mipmaps = 1;

    // Unbind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return target;
}

void EndTextureModeMSAA(RenderTexture2D target, RenderTexture2D resolveTarget) {
    /*BeginTextureMode(resolveTarget);*/
    /*    DrawTexturePro(target.texture,*/
    /*                (Rectangle){ 0.0f, 0.0f, (float)target.texture.width, -(float)target.texture.height },*/
    /*                (Rectangle){ 0.0f, 0.0f, (float)resolveTarget.texture.width, (float)resolveTarget.texture.height
     * },*/
    /*                (Vector2){ 0.0f, 0.0f, }, 0.0f, WHITE);*/
    /*EndTextureMode();*/
    glBindFramebuffer(GL_READ_FRAMEBUFFER, target.id);        // Bind the MSAA framebuffer (read source)
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveTarget.id); // Bind the non-MSAA framebuffer (write target)

    // Blit the multisampled framebuffer to the non-multisampled texture
    glBlitFramebuffer(0, 0, resolveTarget.texture.width, resolveTarget.texture.height, 0, 0,
                      resolveTarget.texture.width, resolveTarget.texture.height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

    // Unbind framebuffer after resolving
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
#else
RenderTexture2D CreateRenderTextureMSAA(int width, int height, int samples) {
    return LoadRenderTexture(width, height);
}

void EndTextureModeMSAA(RenderTexture2D target, RenderTexture2D resolveTarget) {
    BeginTextureMode(resolveTarget);
    DrawTexturePro(target.texture,
                   (Rectangle){0.0f, 0.0f, (float)resolveTarget.texture.width, -(float)resolveTarget.texture.height},
                   (Rectangle){0.0f, 0.0f, (float)resolveTarget.texture.width, (float)resolveTarget.texture.height},
                   Vector2Zero(), 0.0f, WHITE);
    EndTextureMode();
}
#endif

namespace assets {
    Store::Store(Vector2 screen) : texture_map({}), render_map({}), font_map({}) {
        add_textures();
        add_render_textures(screen);
        add_fonts();
        add_models();
    };

    Store::~Store() {
        for (auto& tex : texture_map) {
            UnloadTexture(tex);
        }

        for (auto& render_tex : render_map) {
            UnloadRenderTexture(render_tex);
        }

        for (auto& font : font_map) {
            UnloadFont(font);
        }

        for (auto& model : model_map) {
            UnloadModel(model);
        }
    };

    void Store::draw_texture(RenderId render_id, bool resolved, std::optional<Rectangle> dest) {
        auto tex = (*this)[render_id, resolved].texture;
        DrawTexturePro(tex, (Rectangle){0.0f, 0.0f, static_cast<float>(tex.width), -static_cast<float>(tex.height)},
                       dest ? *dest
                            : (Rectangle){0.0f, 0.0f, static_cast<float>(tex.width), static_cast<float>(tex.height)},
                       Vector2Zero(), 0.0f, WHITE);
    }

    Texture2D Store::operator[](GeneralId id) {
        return texture_map[id];
    }

    Texture2D Store::operator[](spells::Tag name) {
        return texture_map[id_to_idx(name)];
    }

    Texture2D Store::operator[](Rarity id) {
        return texture_map[id_to_idx(id)];
    }

    RenderTexture2D Store::operator[](RenderId id, bool resolved) {
        return index_render_map(id, resolved);
    }

    Font Store::operator[](FontId id) {
        return font_map[id];
    }

    Model Store::operator[](ModelId id) {
        return model_map[id];
    }

    void Store::update_target_size(Vector2 screen) {
        UnloadRenderTexture((*this)[Target, true]);
        UnloadRenderTexture((*this)[Target, false]);
        index_render_map(Target, false) =
            CreateRenderTextureMSAA(static_cast<int>(screen.x), static_cast<int>(screen.y), MSAA);
        index_render_map(Target, true) = LoadRenderTexture(static_cast<int>(screen.x), static_cast<int>(screen.y));
    }

    RenderTexture2D& Store::index_render_map(RenderId id, bool resolved) {
        return render_map[id + (resolved ? static_cast<int>(RenderIdSize) : 0)];
    }

    std::size_t Store::id_to_idx(GeneralId id) {
        return id;
    }

    std::size_t Store::id_to_idx(spells::Tag name) {
        return static_cast<std::size_t>(name) + GeneralIdSize;
    }

    std::size_t Store::id_to_idx(Rarity id) {
        return static_cast<std::size_t>(id) + GeneralIdSize + static_cast<std::size_t>(spells::Tag::Size);
    }

    void Store::add_textures() {
        Image img = LoadImage("./assets/spell-icons/empty-slot.png");
        texture_map[EmptySpellSlot] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImage("./assets/spell-icons/locked-slot.png");
        texture_map[LockedSlot] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImage("./assets/splash-screen.png");
        texture_map[SplashScreen] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImage("./assets/main-menu.png");
        texture_map[MainMenu] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImage("./assets/play-button.png");
        texture_map[PlayButton] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImage("./assets/play-button-hover.png");
        texture_map[PlayButtonHover] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImage("./assets/exit-button.png");
        texture_map[ExitButton] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImage("./assets/exit-button-hover.png");
        texture_map[ExitButtonHover] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImage("./assets/floor.png");
        texture_map[Floor] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImage("./assets/powerup-background.png");
        texture_map[PowerUpBackground] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImage("./assets/arrow.png");
        texture_map[SoulPortalArrow] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImage("./assets/spellbook-background.png");
        texture_map[SpellBookBackground] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImage("./assets/pause-background.png");
        texture_map[PauseBackground] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImage("./assets/hub-background.png");
        texture_map[HubBackground] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImage("./assets/spell-tile-background.png");
        texture_map[SpellTileBackground] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImage("./assets/spell-tile-rarity.png");
        texture_map[SpellTileRarityFrame] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImage("./assets/spellicon-rarity.png");
        texture_map[SpellIconRarityFrame] = LoadTextureFromImage(img);
        UnloadImage(img);

        for (std::size_t tag_i = 0; tag_i < static_cast<int>(spells::Tag::Size); tag_i++) {
            img = LoadImage((std::string(spell::icon_path) + "/" + spells::infos[tag_i].icon).c_str());
            texture_map[tag_i + GeneralIdSize] = LoadTextureFromImage(img);
            UnloadImage(img);
        }

        for (std::size_t rarity_i = 0; rarity_i < static_cast<int>(Rarity::Size); rarity_i++) {
            img = LoadImage((std::string(rarity::frame_path) + "/" + rarity::info[rarity_i].frame).c_str());
            texture_map[rarity_i + static_cast<int>(spells::Tag::Size) + GeneralIdSize] = LoadTextureFromImage(img);
            UnloadImage(img);
        }

        for (auto& t : texture_map) {
            GenTextureMipmaps(&t);
            SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);

            glBindTexture(GL_TEXTURE_2D, t.id);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, -1.5f);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    void Store::add_render_textures(Vector2 screen) {
        index_render_map(Target, false) =
            CreateRenderTextureMSAA(static_cast<int>(screen.x), static_cast<int>(screen.y), MSAA);
        index_render_map(Target, true) = LoadRenderTexture(static_cast<int>(screen.x), static_cast<int>(screen.y));
        index_render_map(CircleUI, false) = CreateRenderTextureMSAA(1023, 1023, MSAA); // odd, to have a center pixel
        index_render_map(CircleUI, true) = LoadRenderTexture(1023, 1023);
    }

    void Store::add_fonts() {
        font_map[Macondo] = LoadFontEx("./assets/Macondo/Macondo-Regular.ttf", 64, nullptr, 0);
    }

    void Store::add_models() {
        model_map[Player] = LoadModel("./assets/player/player.glb");
        model_map[Player].transform =
            MatrixMultiply(model_map[Player].transform, MatrixRotateX(static_cast<float>(std::numbers::pi) / 2.0f));
    }
}
