#include <ecs.hpp>

#include <print>

using ECS = ecs::build<ecs::Archetype<int, float>, ecs::Archetype<int, float, std::string>>;

int main() {
    auto ecs = ECS();

    auto ent1 = ecs.static_emplace_entity<ecs::Archetype<int, float>>(100, 0.111f);
    auto ent2 = ecs.static_emplace_entity<ecs::Archetype<int, float>>(200, 0.222f);
    ecs.extend<std::string>(ent1, std::string("Hello"));

    ecs.make_system<std::string>().run([&](std::string& s) {
        std::println("{}", s);
    });

    ecs.make_system<int, float>().run([&](int& i, float& f) {
        std::println("{}, {}", i, f);
    });

    // ecs.extend<double>(ent, 0.69);
    // found = false;
    // ecs.make_system<double>().run([&](double& d) {
    //     found = true;
    //     REQUIRE(d == 0.69);
    // });
    // REQUIRE(found);

    return 0;
}
