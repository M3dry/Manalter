#include <ecs.hpp>

#include <print>

TAG_BY_NAME(int, funny_int);

using ECS = ecs::build<ecs::Archetype<funny_int, float>, ecs::Archetype<int, char>>;

int main() {
    auto ecs = ECS();

    auto ent1 = ecs.static_emplace_entity<ecs::Archetype<funny_int, float>>(100, 0.111f);
    auto ent2 = ecs.static_emplace_entity<ecs::Archetype<funny_int, float>>(200, 0.222f);
    auto ent3 = ecs.static_emplace_entity<ecs::Archetype<int, char>>(300, 'c');
    auto ent4 = ecs.static_emplace_entity<ecs::Archetype<int, char>>(400, 'h');

    ecs.make_static_system<funny_int>().run([](int& i) {
        std::println("{}", i);
    });

    return 0;
}
