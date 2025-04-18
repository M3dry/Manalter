#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <sstream>
#include <type_traits>

#include "seria_deser.hpp"

TEST_CASE("Int", "[seria_deser]") {
    int x = 10;

    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    seria_deser::serialize(x, ss);

    int deserialized_x = seria_deser::deserialize(ss, CURRENT_VERSION, std::type_identity<int>{});

    REQUIRE(x == deserialized_x);
}

enum class E : uint8_t {
    A = 0,
    B,
    C,
};

TEST_CASE("Enum", "[seria_deser]") {
    E a = E::A;

    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    seria_deser::serialize(a, ss);

    E deserialized_a = seria_deser::deserialize(ss, CURRENT_VERSION, std::type_identity<E>{});

    REQUIRE(a == deserialized_a);
}

struct S {
    E e;
    bool f;
    std::string str;
    std::optional<std::vector<S>> ss;

    bool operator==(const S& other) const {
        if (e != other.e || f != other.f || str != other.str || ss.has_value() != other.ss.has_value()) return false;

        if (!ss.has_value()) return true;

        for (std::size_t i = 0; i < ss->size(); i++) {
            if ((*ss)[i] != (*other.ss)[i]) return false;
        }

        return true;
    }

    void serialize(std::ostream& out) const {
        seria_deser::serialize(e, out);
        seria_deser::serialize(f, out);
        seria_deser::serialize(str, out);
        seria_deser::serialize(ss, out);
    }

    static S deserialize(std::istream& in, version v) {
        return S{
            .e = seria_deser::deserialize(in, v, std::type_identity<E>{}),
            .f = seria_deser::deserialize(in, v, std::type_identity<bool>{}),
            .str = seria_deser::deserialize(in, v, std::type_identity<std::string>{}),
            .ss = seria_deser::deserialize(in, v, std::type_identity<std::optional<std::vector<S>>>{}),
        };
    }
};

TEST_CASE("Struct", "[seria_deser]") {
    S s = S{
        .e = E::A,
        .f = false,
        .str = "skibidi",
        .ss = {{
            S{
                .e = E::B,
                .f = true,
                .str = "bombardiro",
                .ss = std::nullopt,
            },
            S{
                .e = E::C,
                .f = false,
                .str = "crocodilo",
                .ss = std::nullopt,
            },
        }},
    };

    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    seria_deser::serialize(s, ss);

    S deserialized_s = seria_deser::deserialize(ss, CURRENT_VERSION, std::type_identity<S>{});

    REQUIRE(s == deserialized_s);
}
