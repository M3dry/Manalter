#include "spell.hpp"
#include <raylib.h>

spell::Info spells::get_info(const Data &data) {
    return std::visit([](auto&& arg) {
        return std::decay_t<decltype(arg)>::info;
    }, data);
}

Spell Spell::random(uint16_t max_level) {
    return Spell(spells::create_spell(static_cast<spells::Tag>(GetRandomValue(0, static_cast<int>(spells::Tag::Size) - 1))),
                 static_cast<Rarity>(GetRandomValue(0, static_cast<int>(Rarity::Size) - 1)), GetRandomValue(0, max_level));
}

Spell::~Spell() {
    /*UnloadTexture(texture);*/
}
