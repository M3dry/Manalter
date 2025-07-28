#include <ecs.hpp>

#include <print>

using ECS = ecs::build<ecs::Archetype<int, float>, ecs::Archetype<int, char>>;

int main() {
    auto ecs = ECS();

    auto ent1 = ecs.static_emplace_entity<ecs::Archetype<int, float>>(100, 0.111f);
    auto ent2 = ecs.static_emplace_entity<ecs::Archetype<int, float>>(200, 0.222f);
    auto ent3 = ecs.static_emplace_entity<ecs::Archetype<int, char>>(300, 'c');
    auto ent4 = ecs.static_emplace_entity<ecs::Archetype<int, char>>(400, 'h');

    ecs::Entity es[] = {ent1, ent2, ent3, ent4};
    for (std::size_t i = 0; i < 4; i++) {
        std::println("{} {} {}", ecs.is_dirty<int>(es[i]), ecs.is_dirty<char>(es[i]), ecs.is_dirty<int, char>(es[i]));
    }

    ecs.make_system<const int, char>().run<ecs::OnlyDirty|ecs::MarkDirty>([&](const int& i, char& c) {
        std::println("{}", i);
    });

    for (std::size_t i = 0; i < 4; i++) {
        std::println("{} {} {}", ecs.is_dirty<int>(es[i]), ecs.is_dirty<char>(es[i]), ecs.is_dirty<int, char>(es[i]));
    }

    return 0;
}
