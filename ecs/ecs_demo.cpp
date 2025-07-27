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
        std::println("{}", ecs.is_dirty<int>(es[i]));
    }

    ecs.make_system<const int, char>().run<ecs::OnlyDirty|ecs::MarkDirty>([&](const int& i, char& c) {
        std::println("{}", i);
    });

    // ecs.make_system<char>().run<ecs::OnlyDirty>([&](char& i) {
    //     std::println("{}", i);
    // });
    //
    // ecs.make_system<int, char>().run<ecs::OnlyDirty>([&](int& i, char& c) {
    //     assert(false);
    // });
    //
    // ecs.make_system<int, float>().run<ecs::OnlyDirty>([&](int& i, float& f) {
    //     std::println("{} {}", i, f);
    // });

    for (std::size_t i = 0; i < 4; i++) {
        std::println("{}", ecs.is_dirty<int>(es[i]));
    }

    // ecs.make_system<int>()
    //
    // auto ra = ecs.new_archetype<int, bool>();
    // auto ent5 = ecs.emplace_entity(ra, 500, false);
    // auto ent6 = ecs.emplace_entity(ra, 600, true);
    //
    // ecs::Entity es[] = {ent1, ent2, ent3, ent4, ent5, ent6};
    // for (std::size_t i = 0; i < 6; i++) {
    //     auto e = ecs.get<int>(es[i]);
    //     std::println("{}", std::get<0>(*e));
    // }

    return 0;
}
