#include <ecs.hpp>

#include <print>

using ECS = ecs::build<ecs::Archetype<int, float>, ecs::Archetype<int, char>>;

int main() {
    auto ecs = ECS();

    auto ent1 = ecs.static_emplace_entity<ecs::Archetype<int, float>>(100, 0.111f);
    auto ent2 = ecs.static_emplace_entity<ecs::Archetype<int, float>>(200, 0.222f);
    auto ent3 = ecs.static_emplace_entity<ecs::Archetype<int, char>>(300, 'c');
    auto ent4 = ecs.static_emplace_entity<ecs::Archetype<int, char>>(400, 'h');

    ecs.make_system<int>().run([&](int& i) {
        if (i == 100) ecs.static_emplace_entity<ecs::Archetype<int, float>>(500, 0.333f);
        std::println("{}", i);
    });

    std::println("{}", *std::get<0>(*ecs.get<int>(0, ent1)));

    // ecs.make_system<int>().run([&](int& i) {
    //     std::println("{}", i);
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
