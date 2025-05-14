#include "assets.hpp"
#include "raylib.h"
#include "spell.hpp"
#include "texture_includes.hpp"

namespace assets {
    Store::Store(Vector2 screen) : texture_map({}), render_map({}) {
        add_textures();
        add_render_textures(screen);
        add_models();
    };

    Store::~Store() {
        for (auto& tex : texture_map) {
            UnloadTexture(tex);
        }

        for (auto& render_tex : render_map) {
            UnloadRenderTexture(render_tex);
        }

        for (auto& model : model_map) {
            UnloadModel(model);
        }
    };

    void Store::draw_texture(RenderId render_id, std::optional<Rectangle> dest) {
        auto tex = (*this)[render_id].texture;
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

    RenderTexture2D Store::operator[](RenderId id) {
        return index_render_map(id);
    }

    Model Store::operator[](ModelId id) {
        return model_map[id];
    }

    void Store::update_target_size(Vector2 screen) {
        UnloadRenderTexture((*this)[Target]);
        index_render_map(Target) = LoadRenderTexture(static_cast<int>(screen.x), static_cast<int>(screen.y));

        auto& t = index_render_map(Target).texture;
        SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
    }

    RenderTexture2D& Store::index_render_map(RenderId id) {
        return render_map[id];
    }

    std::size_t Store::id_to_idx(GeneralId id) {
        return id;
    }

    std::size_t Store::id_to_idx(spells::Tag name) {
        return static_cast<std::size_t>(name) + GeneralIdSize;
    }

    void Store::add_textures() {
        Image img = LoadImageFromMemory(".png", texture_includes::empty_slot, sizeof(texture_includes::empty_slot));
        texture_map[EmptySpellSlot] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImageFromMemory(".png", texture_includes::locked_slot, sizeof(texture_includes::locked_slot));
        texture_map[LockedSlot] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImageFromMemory(".png", texture_includes::splash_screen, sizeof(texture_includes::splash_screen));
        texture_map[SplashScreen] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImageFromMemory(".png", texture_includes::main_menu, sizeof(texture_includes::main_menu));
        texture_map[MainMenu] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImageFromMemory(".png", texture_includes::play_button, sizeof(texture_includes::play_button));
        texture_map[PlayButton] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImageFromMemory(".png", texture_includes::play_button_hover,
                                  sizeof(texture_includes::play_button_hover));
        texture_map[PlayButtonHover] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImageFromMemory(".png", texture_includes::exit_button, sizeof(texture_includes::exit_button));
        texture_map[ExitButton] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImageFromMemory(".png", texture_includes::exit_button_hover,
                                  sizeof(texture_includes::exit_button_hover));
        texture_map[ExitButtonHover] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImageFromMemory(".png", texture_includes::floor, sizeof(texture_includes::floor));
        texture_map[Floor] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImageFromMemory(".png", texture_includes::powerup_background,
                                  sizeof(texture_includes::powerup_background));
        texture_map[PowerUpBackground] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImageFromMemory(".png", texture_includes::arrow, sizeof(texture_includes::arrow));
        texture_map[SoulPortalArrow] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImageFromMemory(".png", texture_includes::spellbook_background,
                                  sizeof(texture_includes::spellbook_background));
        texture_map[SpellBookBackground] = LoadTextureFromImage(img);
        UnloadImage(img);

        img =
            LoadImageFromMemory(".png", texture_includes::pause_background, sizeof(texture_includes::pause_background));
        texture_map[PauseBackground] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImageFromMemory(".png", texture_includes::hub_background, sizeof(texture_includes::hub_background));
        texture_map[HubBackground] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImageFromMemory(".png", texture_includes::spell_tile_background,
                                  sizeof(texture_includes::spell_tile_background));
        texture_map[SpellTileBackground] = LoadTextureFromImage(img);
        UnloadImage(img);

        img = LoadImageFromMemory(".png", texture_includes::spell_tile_rarity,
                                  sizeof(texture_includes::spell_tile_rarity));
        texture_map[SpellTileRarityFrame] = LoadTextureFromImage(img);
        UnloadImage(img);

        img =
            LoadImageFromMemory(".png", texture_includes::spellicon_rarity, sizeof(texture_includes::spellicon_rarity));
        texture_map[SpellIconRarityFrame] = LoadTextureFromImage(img);
        UnloadImage(img);

        for (std::size_t tag_i = 0; tag_i < static_cast<int>(spells::Tag::Size); tag_i++) {
            img = LoadImageFromMemory(".png", spells::infos[tag_i].icon_data,
                                      static_cast<int>(spells::infos[tag_i].icon_size));
            texture_map[tag_i + GeneralIdSize] = LoadTextureFromImage(img);
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
        index_render_map(Target) = LoadRenderTexture(static_cast<int>(screen.x), static_cast<int>(screen.y));
        index_render_map(CircleUI) = LoadRenderTexture(1023, 1023); // odd, to have a center pixel

        for (auto& t : render_map) {
            SetTextureFilter(t.texture, TEXTURE_FILTER_TRILINEAR);
        }
    }

    void Store::add_models() {
        model_map[Player] = LoadModel("./assets/player/player.glb");
        model_map[Player].transform =
            MatrixMultiply(model_map[Player].transform, MatrixRotateX(static_cast<float>(std::numbers::pi) / 2.0f));
    }
}
