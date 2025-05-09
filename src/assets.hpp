#pragma once

#include <array>
#include <concepts>
#include <optional>

#include <raylib.h>
#include <raymath.h>

#ifdef PLATFORM_WEB
#include <GLES3/gl3.h>
#else
#include "glad.h"
#endif

#include "spell.hpp"

// TODO: make this a runtime variable
#define MSAA 16

RenderTexture2D CreateRenderTextureMSAA(int width, int height, int samples);
void EndTextureModeMSAA(RenderTexture2D target, RenderTexture2D resolveTarget);

namespace assets {
    class Store;

    template <typename T>
    concept ToTexture2D = requires(Store s, T t) {
        { s[t] } -> std::same_as<Texture2D>;
    };

    enum GeneralId {
        EmptySpellSlot = 0,
        LockedSlot,
        SplashScreen,
        MainMenu,
        PlayButton,
        PlayButtonHover,
        ExitButton,
        ExitButtonHover,
        Floor,
        PowerUpBackground,
        SoulPortalArrow,
        SpellBookBackground,
        PauseBackground,
        HubBackground,
        SpellTileBackground,
        SpellTileRarityFrame,
        SpellIconRarityFrame,
        GeneralIdSize,
    };

    enum RenderId {
        Target = 0,
        CircleUI,
        RenderIdSize,
    };

    enum FontId {
        Macondo,
        FontSize,
    };

    enum ModelId {
        Player,
        ModelSize,
    };

    class Store {
      public:
        Store(Vector2 screen);
        Store(const Store&) = delete;
        ~Store();

        template <ToTexture2D Id> void draw_texture(Id texture_id, std::optional<Rectangle> dest = std::nullopt) {
            auto tex = (*this)[texture_id];
            DrawTexturePro(
                tex, (Rectangle){0.0f, 0.0f, static_cast<float>(tex.width), static_cast<float>(tex.height)},
                dest ? *dest : (Rectangle){0.0f, 0.0f, static_cast<float>(tex.width), static_cast<float>(tex.height)},
                Vector2Zero(), 0.0f, WHITE);
        }

        void draw_texture(RenderId render_id, bool resolved, std::optional<Rectangle> dest);

        Texture2D operator[](GeneralId id);
        Texture2D operator[](spells::Tag name);
        Texture2D operator[](Rarity id);
        RenderTexture2D operator[](RenderId id, bool resolved);
        Font operator[](FontId id);
        Model operator[](ModelId id);

        void update_target_size(Vector2 screen);

      private:
        // order: GeneralIds, Names, Rarities
        std::array<Texture2D, static_cast<int>(GeneralIdSize) + static_cast<int>(spells::Tag::Size) +
                                  static_cast<int>(Rarity::Size)>
            texture_map;
        std::array<RenderTexture2D, RenderIdSize * 2> render_map;
        std::array<Font, FontSize> font_map;
        std::array<Model, ModelSize> model_map;

        RenderTexture2D& index_render_map(RenderId id, bool resolved);

        std::size_t id_to_idx(GeneralId id);
        std::size_t id_to_idx(spells::Tag name);
        std::size_t id_to_idx(Rarity id);

        void add_textures();
        void add_render_textures(Vector2 screen);
        void add_fonts();
        void add_models();
    };
}
