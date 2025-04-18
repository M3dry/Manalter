#pragma once

#include <cassert>
#include <cstdint>
#include <istream>
#include <limits>
#include <ostream>
#include <type_traits>
#include <vector>

using version = uint32_t;
inline constexpr version CURRENT_VERSION = 1;

namespace seria_deser {
    template <typename T>
    concept _SeriaDeser = requires(T& t, std::ostream& out, std::istream& in, version v) {
        { t.serialize(out) } -> std::same_as<void>;
        { T::deserialize(in, v) } -> std::same_as<T>;
    };

    template <typename T>
        requires std::is_integral_v<T>
    inline void serialize(const T& t, std::ostream& out) {
        out.write(reinterpret_cast<const char*>(&t), sizeof(T));
    }

    template <typename Enum>
        requires std::is_enum_v<Enum>
    inline void serialize(const Enum& e, std::ostream& out) {
        using Underlying = std::underlying_type_t<Enum>;
        serialize(static_cast<Underlying>(e), out);
    }

    template <_SeriaDeser T> inline void serialize(const T& t, std::ostream& out) {
        t.serialize(out);
    }

    template <typename T> void serialize(const std::vector<T>& vec, std::ostream& out) {
        auto size = vec.size();
        serialize(size, out);

        for (const T& x : vec) {
            serialize(x, out);
        }
    }

    template <typename T> void serialize(const std::optional<T>& opt, std::ostream& out) {
        serialize(opt.has_value(), out);
        if (opt) {
            serialize(*opt, out);
        }
    }

    inline void serialize(const std::string& s, std::ostream& out) {
        // NOTE: should this be fixed??? I mean we def not gonna serialize that much string
        auto size = s.size();
        assert(size <= std::numeric_limits<long>::max() && "yeah.... how");
        serialize(size, out);
        out.write(s.data(), static_cast<long>(size));
    }

    template <typename T>
        requires std::is_integral_v<T>
    inline T deserialize(std::istream& in, version _, std::type_identity<T>) {
        T i;
        in.read(reinterpret_cast<char*>(&i), sizeof(T));

        return i;
    }

    template <typename Enum>
        requires std::is_enum_v<Enum>
    inline Enum deserialize(std::istream& in, version _, std::type_identity<Enum>) {
        using Underlying = std::underlying_type_t<Enum>;
        Underlying val;

        in.read(reinterpret_cast<char*>(&val), sizeof(Underlying));

        return static_cast<Enum>(val);
    }

    template <_SeriaDeser T> inline T deserialize(std::istream& in, version v, std::type_identity<T>) {
        return T::deserialize(in, v);
    }

    template <typename T> std::vector<T> deserialize(std::istream& in, version v, std::type_identity<std::vector<T>>) {
        std::size_t size = deserialize(in, v, std::type_identity<std::size_t>{});

        std::vector<T> vec;
        for (std::size_t i = 0; i < size; i++) {
            vec.emplace_back(deserialize(in, v, std::type_identity<T>{}));
        }

        return vec;
    }

    template <typename T>
    std::optional<T> deserialize(std::istream& in, version v, std::type_identity<std::optional<T>>) {
        bool has_value = deserialize(in, v, std::type_identity<bool>{});

        if (!has_value) return std::nullopt;

        return deserialize(in, v, std::type_identity<T>{});
    }

    inline std::string deserialize(std::istream& in, version v, std::type_identity<std::string>) {
        std::size_t size = deserialize(in, v, std::type_identity<std::size_t>{});
        assert(size <= std::numeric_limits<long>::max());

        long _size = static_cast<long>(size);
        std::string str(static_cast<std::size_t>(_size), '\0');
        in.read(str.data(), _size);

        return str;
    }

    template <typename T> T deserialize(std::istream& in, version v) {
        return deserialize(in, v, std::type_identity<T>{});
    }

    inline version deserialize_version(std::istream& in) {
        return deserialize<version>(in, static_cast<version>(-1));
    }
}

template <typename T>
concept SeriaDeser = requires(const T& t, std::ostream& out, std::istream& in, version v) {
    { seria_deser::serialize(t, out) } -> std::same_as<void>;
    { seria_deser::deserialize(in, v, std::type_identity<T>{}) } -> std::same_as<T>;
};
