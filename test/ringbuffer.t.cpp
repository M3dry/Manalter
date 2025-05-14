#include <catch2/catch_test_macros.hpp>
#include <ranges>
#include <sstream>

#include "ringbuffer.hpp"
#include "seria_deser.hpp"

TEST_CASE("Test", "[ringbuffer]") {
    RingBuffer<int> rb;
    rb.emplace_back(10);
    rb.emplace_front(20);
    rb.emplace_front(30);
    rb.emplace_back(40);

    int expected[] = {30, 20, 10, 40};
    for (const auto& [i, x] : rb | std::ranges::views::enumerate) {
        REQUIRE(expected[i] == x);
    }

    rb.pop_front();
    rb.pop_front();
    rb.pop_back();

    REQUIRE(rb.size() == 1);
    REQUIRE(rb[0] == 10);
    REQUIRE(rb.front() == rb.back());
}

TEST_CASE("Push and access", "[ringbuffer]") {
    RingBuffer<std::string> rb;
    rb.emplace_back("one");
    rb.emplace_back("two");
    rb.emplace_back("three");

    REQUIRE(rb.size() == 3);
    REQUIRE(rb.front() == "one");
    REQUIRE(rb.back() == "three");
    REQUIRE(rb[1] == "two");
}

TEST_CASE("Wraparound behavior", "[ringbuffer]") {
    RingBuffer<int> rb(4);

    rb.emplace_back(1);
    rb.emplace_back(2);
    rb.emplace_back(3);
    rb.emplace_back(4);
    rb.pop_front();
    rb.emplace_back(5);

    int expected[] = {2, 3, 4, 5};
    for (const auto& [i, x] : rb | std::views::enumerate) {
        REQUIRE(expected[i] == x);
    }
}

TEST_CASE("Push to front and back", "[ringbuffer]") {
    RingBuffer<int> rb;

    rb.emplace_front(3);
    rb.emplace_front(2);
    rb.emplace_front(1);
    rb.emplace_back(4);
    rb.emplace_back(5);

    int expected[] = {1, 2, 3, 4, 5};
    for (const auto& [i, x] : rb | std::views::enumerate) {
        REQUIRE(expected[i] == x);
    }
}

TEST_CASE("Clear and reuse", "[ringbuffer]") {
    RingBuffer<int> rb;

    rb.emplace_back(1);
    rb.emplace_back(2);
    rb.emplace_back(3);

    while (!rb.empty()) {
        rb.pop_back();
    }

    REQUIRE(rb.empty());
    rb.emplace_back(10);
    rb.emplace_front(9);
    REQUIRE(rb.front() == 9);
    REQUIRE(rb.back() == 10);
}

TEST_CASE("Iterator traversal", "[ringbuffer]") {
    RingBuffer<int> rb;
    for (int i = 0; i < 5; ++i)
        rb.emplace_back(i);

    int expected = 0;
    for (auto it = rb.begin(); it != rb.end(); ++it) {
        REQUIRE(*it == expected++);
    }
}

TEST_CASE("Serialization and deserialization", "[ringbuffer][seria_deser]") {
    RingBuffer<int> rb;

    rb.push_back(10);
    rb.push_front(0);
    rb.push_back(100);

    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    seria_deser::serialize(rb, ss);

    auto deserialized_rb = seria_deser::deserialize<RingBuffer<int>>(ss, CURRENT_VERSION);

    REQUIRE(rb.size() == deserialized_rb.size());

    for (std::size_t i = 0; i < rb.size(); i++) {
        REQUIRE(rb[i] == deserialized_rb[i]);
    }
}

TEST_CASE("Removal", "[ringbuffer]") {
    RingBuffer<int> rb;

    rb.push_back(10);
    rb.push_back(20);
    rb.push_back(30);
    rb.push_back(40);
    rb.push_back(50);

    rb.remove(1); // 20
    rb.remove(2); // 40
    int expected[] = {10, 30, 50};
    REQUIRE(rb.size() == sizeof(expected));
    for (std::size_t i = 0; i < rb.size(); i++) {
        REQUIRE(rb[i] == expected[i]);
    }
}
