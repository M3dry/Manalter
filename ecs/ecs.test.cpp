#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <ecs.hpp>
#include <string>
#include <vector>
#include <typeindex>

// Utilities
using ECS = ecs::build<ecs::Archetype<int, float>, ecs::Archetype<int, float, std::string>>;

TEST_CASE("Static entity emplace and retrieval", "[ecs][static]") {
    auto ecs = ECS();

    auto ent = ecs.static_emplace_entity<ecs::Archetype<int, float>>(42, 3.14f);

    bool found = false;
    ecs.make_system<int, float>().run([&](int& i, float& f) {
        found = true;
        REQUIRE(i == 42);
        REQUIRE_THAT(f, Catch::Matchers::WithinRel(3.14f));
    });
    REQUIRE(found);
}

TEST_CASE("Dynamic archetype creation and use", "[ecs][dynamic]") {
    auto ecs = ECS();

    std::size_t arch_id = ecs.new_archetype<int, std::string>();
    REQUIRE(ecs.is_runtime_archetype(arch_id));

    auto ent = ecs.emplace_entity(arch_id, 99, std::string("hello"));

    bool matched = false;
    ecs.make_system<int, std::string>().run([&](int& i, std::string& s) {
        matched = true;
        REQUIRE(i == 99);
        REQUIRE(s == "hello");
    });
    REQUIRE(matched);
}

TEST_CASE("static_extend adds components correctly", "[ecs][extend]") {
    auto ecs = ECS();

    auto ent = ecs.static_emplace_entity<ecs::Archetype<int, float>>(123, 9.81f);
    ecs.static_extend<std::string>(ent, "gravity");

    bool extended = false;
    ecs.make_system<int, float, std::string>().run([&](int& i, float& f, std::string& s) {
        extended = true;
        REQUIRE(i == 123);
        REQUIRE_THAT(f, Catch::Matchers::WithinRel(9.81f));
        REQUIRE(s == "gravity");
    });
    REQUIRE(extended);
}

TEST_CASE("set_entity moves an existing entity to new archetype", "[ecs][migrate]") {
    auto ecs = ECS();

    auto ent = ecs.new_entity();
    ecs.set_entity(ecs.new_archetype<int, std::string>(), ent, 7, std::string("moved"));

    bool ran = false;
    ecs.make_system<int, std::string>().run([&](int& i, std::string& s) {
        ran = true;
        REQUIRE(i == 7);
        REQUIRE(s == "moved");
    });
    REQUIRE(ran);
}

TEST_CASE("remove works and find_dead detects it", "[ecs][remove][lifecycle]") {
    auto ecs = ECS();
    auto ent = ecs.static_emplace_entity<ecs::Archetype<int, float>>(11, 1.1f);
    ecs.remove(ent);

    bool dead_found = false;
    ecs.find_dead([&](ecs::Entity e) {
        dead_found = (e == ent);
    });

    REQUIRE(dead_found);
}

TEST_CASE("archetype_exists correctly identifies archetypes", "[ecs][meta]") {
    auto ecs = ECS();

    auto id = ecs.new_archetype<int, float, std::string>();

    std::vector<std::type_index> types = {typeid(int), typeid(float), typeid(std::string)};
    auto opt_id = ecs.archetype_exists(types);
    REQUIRE(opt_id.has_value());
    REQUIRE(opt_id.value() == id);
}

TEST_CASE("Dynamic set_entity with raw void* arguments", "[ecs][dynamic][unsafe]") {
    auto ecs = ECS();
    auto arch_id = ecs.new_archetype<int, std::string>();
    auto ent = ecs.new_entity();

    int i = 123;
    std::string s = "unsafe";
    void* args[] = { &i, &s };

    ecs.dynamic_set_entity(arch_id, ent, args, 2);

    bool matched = false;
    ecs.make_system<int, std::string>().run([&](int& i2, std::string& s2) {
        matched = true;
        REQUIRE(i2 == 123);
        REQUIRE(s2 == "unsafe");
    });

    REQUIRE(matched);
}

TEST_CASE("Systems can operate on subsets and supersets", "[ecs][system][subset]") {
    auto ecs = ECS();

    ecs.static_emplace_entity<ecs::Archetype<int, float, std::string>>(10, 1.5f, "superset");

    bool base_run = false;
    ecs.make_system<int>().run([&](int& i) {
        base_run = true;
        REQUIRE(i == 10);
    });
    REQUIRE(base_run);

    bool strict_run = false;
    ecs.make_system<int, float, std::string>().run([&](int& i, float& f, std::string& s) {
        strict_run = true;
        REQUIRE(i == 10);
        REQUIRE_THAT(f, Catch::Matchers::WithinRel(1.5f));
        REQUIRE(s == "superset");
    });
    REQUIRE(strict_run);
}

struct Tracker {
    inline static int counter = 0;
    Tracker() { ++counter; }
    Tracker(const Tracker&) { ++counter; }
    Tracker(Tracker&&) noexcept { ++counter; }
    Tracker& operator=(Tracker&&) noexcept { ++counter; return *this; };
    ~Tracker() { --counter; }
};
TEST_CASE("Resource destruction and RAII verification", "[ecs][lifecycle]") {
    {
        auto ecs = ecs::build<ecs::Archetype<Tracker>>();
        ecs.static_emplace_entity<ecs::Archetype<Tracker>>(Tracker{});
        REQUIRE(Tracker::counter >= 1);

        ecs.make_system<Tracker>().run([](Tracker&) {});
    }

    REQUIRE(Tracker::counter == 0);  // All Trackers should be destroyed
}

TEST_CASE("Double removal does not crash", "[ecs][robustness]") {
    auto ecs = ECS();
    auto ent = ecs.static_emplace_entity<ecs::Archetype<int, float>>(10, 0.1f);

    ecs.remove(ent);
    REQUIRE_NOTHROW(ecs.remove(ent)); // Removing twice shouldn't throw
}

TEST_CASE("System with no matching entities does not invoke callback", "[ecs][system][empty]") {
    auto ecs = ECS();
    bool called = false;
    ecs.make_system<int, std::string>().run([&](int&, std::string&) {
        called = true;
    });
    REQUIRE_FALSE(called);
}

TEST_CASE("Archetype creation and existence query match", "[ecs][meta][runtime]") {
    auto ecs = ECS();

    std::type_index types[] = {typeid(std::string), typeid(float)};
    REQUIRE_FALSE(ecs.archetype_exists({types, 2}).has_value());

    auto id = ecs.new_archetype<std::string, float>();
    auto exists = ecs.archetype_exists(types);
    REQUIRE(exists.has_value());
    REQUIRE(exists.value() == id);
}

TEST_CASE("dynamic_extend", "[ecs][extend]") {
    auto ecs = ECS();

    auto ent = ecs.static_emplace_entity<ecs::Archetype<int, float>>(100, 0.111f);
    ecs.extend<std::string>(ent, std::string("Hello"));

    bool found = false;
    ecs.make_system<std::string>().run([&](std::string& s) {
        found = true;
        REQUIRE(s == "Hello");
    });
    REQUIRE(found);

    ecs.extend<double>(ent, 0.69);
    found = false;
    ecs.make_system<double>().run([&](double& d) {
        found = true;
        REQUIRE(d == 0.69);
    });
    REQUIRE(found);
}
