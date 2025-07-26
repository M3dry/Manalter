#include <ecs.hpp>

#include <print>

using ECS = ecs::build<ecs::Archetype<int, float>, ecs::Archetype<int, char>>;

int main() {
    auto ecs = ECS();

    auto ent1 = ecs.static_emplace_entity<ecs::Archetype<int, float>>(100, 0.111f);
    auto ent2 = ecs.static_emplace_entity<ecs::Archetype<int, float>>(200, 0.222f);
    auto ent3 = ecs.static_emplace_entity<ecs::Archetype<int, char>>(300, 'c');
    auto ent4 = ecs.static_emplace_entity<ecs::Archetype<int, char>>(400, 'h');

    auto ra = ecs.new_archetype<int, bool>();
    auto ent5 = ecs.emplace_entity(ra, 500, false);
    auto ent6 = ecs.emplace_entity(ra, 600, true);

    ecs::Entity es[] = {ent1, ent2, ent3, ent4, ent5, ent6};
    for (std::size_t i = 0; i < 6; i++) {
        auto e = ecs.get<int>(es[i]);
        std::println("{}", std::get<0>(*e));
    }

    // ecs.make_system<int>().run([&](int& i) {
    //     if (i == 100) ecs.static_emplace_entity<ecs::Archetype<int, float>>(500, 0.333f);
    //     std::println("{}", i);
    // });
    //
    //
    // std::println("{}", std::get<0>(*ecs.get<int>(0, ent1)));
    //
    // auto s = ecs.make_system_query<ecs::Archetype<int, char>>();
    // s.run<ecs::WithIDs | ecs::SafeInsert>([&](ecs::Entity e, int& i, char& c) {
    //     std::println("{} {}", i, c);
    //     if (i == 300) ecs.static_emplace_entity<ecs::Archetype<int, char>>(69, 'a');
    //
    //     i = static_cast<int>(e);
    // });
    //
    // s.run<ecs::WithIDs>([](ecs::Entity e, int& i, char& c) {
    //     std::println("[{}]: {} {}", e, i, c);
    // });

    // ecs.extend<double>(ent, 0.69);
    // found = false;
    // ecs.make_system<double>().run([&](double& d) {
    //     found = true;
    //     REQUIRE(d == 0.69);
    // });
    // REQUIRE(found);

    return 0;
}
