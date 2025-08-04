#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "multiarray.hpp"
#include "typeset.hpp"

namespace ecs {
    static constexpr std::size_t null_id = std::size_t(-1);

    namespace __ {
        template <std::size_t... Is, typename F>
        constexpr decltype(auto) with_index_sequence(std::index_sequence<Is...>, F&& f) {
            return f(std::integral_constant<std::size_t, Is>{}...);
        }

        template <std::size_t... Is, typename F> constexpr bool for_each_index(std::index_sequence<Is...>, F&& f) {
            return (f(std::integral_constant<std::size_t, Is>{}) || ...);
        }

        template <typename... Pack, typename F> constexpr bool for_each(std::in_place_type_t<Pack...>, F&& f) {
            return (f(std::in_place_type_t<Pack>{}) || ...);
        }

        template <typename... Pack, typename... Pack2, typename F>
            requires(sizeof...(Pack) == sizeof...(Pack2))
        constexpr bool for_each(std::in_place_type_t<Pack...>, std::in_place_type_t<Pack2...>, F&& f) {
            return (f(std::in_place_type_t<Pack>{}, std::in_place_type_t<Pack2>{}) || ...);
        }
    }

    using Entity = uint64_t;

    // if row == -1 and archetype_id == -1, then entity has been deleted
    struct ArchetypeLink {
        std::size_t archetype_id;
        std::size_t row;
    };

    namespace runtime {
        using Dtor = void(void*);
        using MoveCtor = void(void*, void*);

        template <typename T> void generic_dtor(void* t) {
            std::destroy_at(reinterpret_cast<T*>(t));
        }

        template <typename T> void generic_move_ctor(void* dest, void* src) {
            if constexpr (std::is_move_constructible_v<T>) {
                new (dest) T(std::move(*reinterpret_cast<T*>(src)));
            } else {
                new (dest) T(*reinterpret_cast<T*>(src));
            }
        }
    }

    template <typeset::unique_v... Components> struct Archetype {
        struct Backlink {
            Entity entity;
            inline constexpr operator Entity() {
                return entity;
            }
        };

        template <typename T> struct Bitset {
            uint64_t n;
        };

        multi_vector<Components..., Backlink> components;
        multi_vector<Bitset<Components>...> dirty;

        Archetype() {};

        multi_vector<std::size_t, std::type_index, runtime::Dtor*, runtime::MoveCtor*>
        extend(multi_vector<std::size_t, std::type_index, runtime::Dtor*, runtime::MoveCtor*>& extra_mv) {
            multi_vector<std::size_t, std::type_index, runtime::Dtor*, runtime::MoveCtor*> mv(sizeof...(Components) +
                                                                                              extra_mv.size());
            (mv.emplace_back(sizeof(Components), typeid(Components), &runtime::generic_dtor<Components>,
                             &runtime::generic_move_ctor<Components>),
             ...);
            mv.append(extra_mv);

            return mv;
        };

        template <typename... Subset> void mark_dirty(std::size_t row, bool state = true) {
            auto bitsets = dirty.template get<Bitset<Subset>...>(row / 64, std::integral_constant<bool, true>{});
            auto bit = row % 64;

            uint64_t mask = 1ULL << bit;
            ((std::get<Bitset<Subset>&>(bitsets).n =
                  (std::get<Bitset<Subset>&>(bitsets).n & ~mask) | (-static_cast<uint64_t>(state) & mask)),
             ...);
        }

        template <> void mark_dirty<>(std::size_t row, bool state) {
            return mark_dirty<Components...>(row, state);
        }

        template <typename... Subset> void mark_dirty(std::size_t start, std::size_t end, bool state = true) {
            auto bitsets = dirty.template get_span<Bitset<Subset>...>(std::integral_constant<bool, true>{});

            const std::size_t start_bit = start % 64;
            const std::size_t end_bit = end % 64;
            const std::size_t start_word = start / 64;
            const std::size_t end_word = end / 64;

            const uint64_t full_mask = ~0ULL;
            const uint64_t start_mask = full_mask << start_bit;
            const uint64_t end_mask = (end_bit == 0) ? 0 : (full_mask >> (64 - end_bit));
            const bool single_word = (start_word == end_word);

            const uint64_t first_word_mask = single_word ? (start_mask & end_mask) : start_mask;

            if (state) {
                ((std::get<std::span<Bitset<Subset>>>(bitsets)[start_word].n |= first_word_mask), ...);
            } else {
                ((std::get<std::span<Bitset<Subset>>>(bitsets)[start_word].n &= ~first_word_mask), ...);
            }

            if (!single_word) {
                if (state) {
                    ((std::get<std::span<Bitset<Subset>>>(bitsets)[start_word].n |= end_mask), ...);
                } else {
                    ((std::get<std::span<Bitset<Subset>>>(bitsets)[start_word].n &= ~end_mask), ...);
                }

                std::size_t full_word_start = start_word + 1;
                std::size_t full_word_end = end_word;

                if (full_word_start < full_word_end) {
                    (std::memset(&std::get<std::span<Bitset<Subset>>>(bitsets)[full_word_start], state ? 0xFF : 0x00,
                                 sizeof(uint64_t) * (full_word_end - full_word_start)),
                     ...);
                }
            }
        }

        template <> void mark_dirty<>(std::size_t start, std::size_t end, bool state) {
            return mark_dirty<Components...>(start, end, state);
        }

        template <typename... Subset> bool get_dirty(std::size_t row) {
            auto bit = row % 64;
            auto bitsets = dirty.template get<Bitset<Subset>...>(row / 64, std::integral_constant<bool, true>{});

            return (((std::get<Bitset<Subset>&>(bitsets).n >> bit) & 1) || ...);
        }

        template <> bool get_dirty<>(std::size_t row) {
            return get_dirty<Components...>(row);
        }

        template <typename... Subset> inline std::array<uint64_t**, sizeof...(Subset)> get_dirty_ptrs() {
            auto t = dirty.template get_ptr<Bitset<Subset>...>(std::integral_constant<bool, true>{});

            return __::with_index_sequence(std::index_sequence_for<Subset...>{}, [&](auto... Is) {
                std::array<uint64_t**, sizeof...(Subset)> arr = {reinterpret_cast<uint64_t**>(std::get<Is>(t))...};
                return arr;
            });
        }

        template <typename... Ts> decltype(auto) get(std::size_t row) {
            if constexpr (sizeof...(Ts) == 0) {
                return components.template get<Components...>(row);
            } else if constexpr (sizeof...(Ts) == 1) {
                return components.template get<Ts...>(row);
            } else {
                return components.template get<Ts...>(row);
            }
        }

        template <typename... Packs> void emplace_at(std::size_t row, Packs&&... packs) {
            components.emplace_at(row, std::forward<Packs>(packs)..., components.template get<Backlink>(row).entity);
        }

        void emplace_at(std::size_t row, void* args[], std::size_t nargs) {
            assert(nargs == sizeof...(Components));
            auto t = components.template get_ptr<Components...>(row);

            __::for_each_index(std::index_sequence_for<Components...>{}, [&](auto I) {
                constexpr auto Ix = I.value;
                using T = get_type_t<typeset::nth_t<Ix, Components...>>;

                if constexpr (std::is_move_constructible_v<T>) {
                    std::construct_at(reinterpret_cast<T*>(std::get<Ix>(t)),
                                      std::move(*reinterpret_cast<T*>(args[Ix])));
                } else {
                    std::construct_at(reinterpret_cast<T*>(std::get<Ix>(t)), *reinterpret_cast<T*>(args[Ix]));
                }

                return false;
            });

            mark_dirty(row);
        }

        template <typename... Packs>
        void new_entity(std::vector<ArchetypeLink>& link, Entity entity, Packs&&... packs) {
            components.emplace_back(std::forward<Packs>(packs)..., entity);

            link[entity].row = components.size() - 1;

            if (components.size() > dirty.size() * 64) dirty.emplace_back((typeset::void_f<Components>::f(), 0ULL)...);
            mark_dirty(link[entity].row);
        };

        void new_entity(std::vector<ArchetypeLink>& link, Entity entity, void* args[], std::size_t nargs) {
            auto t = components.unsafe_push();
            emplace_at(components.size() - 1, args, nargs);

            link[entity].row = components.size() - 1;
            std::get<Backlink*>(t)->entity = entity;

            if (components.size() > dirty.size() * 64) dirty.emplace_back((typeset::void_f<Components>::f(), 0ULL)...);
            mark_dirty(link[entity].row);
        }

        std::tuple<get_type_t<Components>*...> unsafe_push_entity(std::vector<ArchetypeLink>& link, Entity entity) {
            auto t = components.unsafe_push();

            link[entity].row = components.size() - 1;
            std::get<Backlink*>(t)->entity = entity;

            if (components.size() > dirty.size() * 64) dirty.emplace_back((typeset::void_f<Components>::f(), 0ULL)...);
            return std::make_tuple(std::get<get_type_t<Components>*>(t)...);
        }

        std::tuple<get_type_t<Components>*...> get_row(std::size_t row) {
            return components.template get_ptr<Components...>(row);
        }

        std::vector<void*> get_row_vec(std::size_t row,
                                       std::span<std::type_index> subset = {(std::type_index*)nullptr, 0}) {
            auto t = get_row(row);
            std::vector<void*> res{};
            res.reserve(subset.size() != 0 ? subset.size() : sizeof...(Components));

            if (subset.size() == 0) {
                __::for_each_index(std::index_sequence_for<Components...>{}, [&](auto I) {
                    constexpr auto Ix = I.value;

                    res.emplace_back(std::get<Ix>(t));
                    return false;
                });
            } else {
                __::for_each_index(std::index_sequence_for<Components...>{}, [&](auto I) {
                    constexpr auto Ix = I.value;

                    bool found = false;
                    for (const auto& t : subset) {
                        if (typeid(typeset::nth_t<Ix, Components...>) == t) {
                            found = true;
                            break;
                        }
                    }
                    if (found) res.emplace_back(std::get<Ix>(t));
                    return false;
                });
            }

            return res;
        }

        void remove(std::vector<ArchetypeLink>& link, std::size_t row) {
            (mark_dirty<Components>(row, get_dirty(components.size() - 1)), ...);

            components.swap(row, components.size() - 1);
            components.pop_back();

            link[components.template get<Backlink>(row).entity].row = row;
        }

        template <typename... Subset>
            requires(typeset::in_set_v<Subset, Components...> && ...)
        std::array<void**, sizeof...(Components) + 1> get_subset() {
            auto subset = components.template get_ptr<Subset...>();

            std::array<void**, sizeof...(Components) + 1> arr;
            arr[0] = reinterpret_cast<void**>(components.unsafe_size_ptr());

            if constexpr (sizeof...(Subset) > 1) {
                __::with_index_sequence(std::index_sequence_for<Subset...>{}, [&](auto... Is) {
                    ((arr[Is + 1] = reinterpret_cast<void**>(std::get<Is>(subset))), ...);
                });
            } else {
                arr[1] = reinterpret_cast<void**>(subset);
            }

            return arr;
        }

        std::vector<void**> dynamic_get_subset(const std::span<std::type_index>& subset_ts) {
            std::vector<void**> ret{};
            ret.reserve(subset_ts.size() + 1);

            ret.emplace_back((void**)components.unsafe_size_ptr());

            auto subset = components.get_dynamic(subset_ts);
            ret.insert(ret.end(), subset.begin(), subset.end());

            assert(ret.size() == subset_ts.size() + 1);
            return ret;
        }

        Entity** get_backlink() {
            return (Entity**)components.template get_ptr<Backlink>();
        }
    };

    namespace runtime {
        class Archetype {
          public:
            Archetype(multi_vector<std::size_t, std::type_index, Dtor*, MoveCtor*>&& ci)
                : capacity(0), rows(0), component_info(std::move(ci)), largest_size(0) {
                capacity = 5;

                for (const auto& size : component_info.get_span<std::size_t>()) {
                    components.emplace_back(static_cast<std::byte*>(::operator new(capacity * size)));

                    if (size > largest_size) largest_size = size;
                }
            };

            Archetype(const Archetype&) = delete;
            Archetype& operator=(const Archetype&) = delete;

            Archetype(Archetype&& ra) noexcept
                : capacity(ra.capacity), rows(ra.rows), components(std::move(ra.components)),
                  component_info(std::move(ra.component_info)), largest_size(ra.largest_size),
                  backlink(std::move(ra.backlink)) {
                ra.moved = true;
            };

            ~Archetype() {
                if (moved) return;

                for (std::size_t i = 0; i < components.size(); i++) {
                    ::operator delete(components[i]);
                }

                // TODO: figure out how to get entities into scope to remove free entities in backlink
            }

            multi_vector<std::size_t, std::type_index, Dtor*, MoveCtor*>
            extend(multi_vector<std::size_t, std::type_index, Dtor*, MoveCtor*>& extra_mv) {
                auto component_info_copy = component_info;

                component_info_copy.append(extra_mv);

                return component_info_copy;
            };

            void emplace_at(std::size_t row, void* const* args, std::size_t nargs) {
                assert(nargs == components.size() && "TODO: maybe exceptions");

                for (const auto [i, size] : component_info.get_span<std::size_t>() | std::views::enumerate) {
                    std::memcpy(
                        reinterpret_cast<void*>(std::uintptr_t(components[static_cast<std::size_t>(i)]) + row * size),
                        args[i], size);
                }
            };

            void new_entity(std::vector<ArchetypeLink>& link, Entity entity, void* args[], std::size_t nargs) {
                assert(nargs == components.size() && "TODO: maybe exceptions");

                if (rows >= capacity) resize(capacity * 2);

                auto [sizes, move_ctor] = component_info.get_span<std::size_t, MoveCtor*>();
                for (std::size_t i = 0; i < sizes.size(); i++) {
                    auto addr = components[i] + rows * sizes[i];

                    move_ctor[i](addr, args[i]);
                }

                link[entity].row = rows;
                backlink.emplace_back(entity);
                rows++;
            }

            std::vector<void*> unsafe_push_entity(std::vector<ArchetypeLink>& link, Entity entity) {
                if (rows >= capacity) resize(capacity * 2);

                std::vector<void*> res{};
                res.reserve(components.size());

                auto sizes = component_info.get_span<std::size_t>();
                for (std::size_t i = 0; i < components.size(); i++) {
                    res.emplace_back(reinterpret_cast<void*>(components[i] + rows * sizes[i]));
                }

                rows++;
                link[entity].row = rows - 1;
                backlink.emplace_back(entity);

                return res;
            }

            std::vector<void*> get_row(std::size_t row,
                                       std::span<std::type_index> subset = {(std::type_index*)nullptr, 0}) {
                std::vector<void*> res{};
                res.reserve(subset.size() != 0 ? subset.size() : components.size());

                auto [sizes, types] = component_info.get_span<std::size_t, std::type_index>();
                if (subset.size() == 0) {
                    for (std::size_t i = 0; i < components.size(); i++) {
                        res.emplace_back(reinterpret_cast<void*>(components[i] + row * sizes[i]));
                    }
                } else {
                    for (std::size_t i = 0; i < components.size(); i++) {
                        bool found = false;
                        for (const auto& t : subset) {
                            if (t == types[i]) {
                                found = true;
                                break;
                            }
                        }

                        if (found) res.emplace_back(reinterpret_cast<void*>(components[i] + row * sizes[i]));
                    }
                }

                assert(res.size() == subset.size() || res.size() == components.size());
                return res;
            }

            void remove(std::vector<ArchetypeLink>& link, std::size_t row) {
                auto [sizes, types, dtor, move_ctor] =
                    component_info.get_span<std::size_t, std::type_index, Dtor*, MoveCtor*>();
                for (std::size_t i = 0; i < sizes.size(); i++) {
                    auto r = components[i] + row * sizes[i];
                    auto e = components[i] + (rows - 1) * sizes[i];

                    dtor[i](r);
                    move_ctor[i](r, e);
                }
                rows--;

                backlink.emplace_at(row, backlink.get<Entity>(row));
                backlink.pop_back();

                link[backlink.get<Entity>(row)].row = row;
            }

            std::vector<void**> get_subset(const std::span<std::type_index>& subset) {
                std::vector<void**> v{};
                v.reserve(subset.size() + 1);
                v.emplace_back(reinterpret_cast<void**>(&rows));

                auto types = component_info.get_span<std::type_index>();
                for (std::size_t s = 0; s < subset.size(); s++) {
                    for (std::size_t i = 0; i < components.size(); i++) {
                        if (types[i] == subset[s]) {
                            v.emplace_back(reinterpret_cast<void**>(&components[i]));
                            break;
                        }
                    }
                }

                assert(v.size() == subset.size() + 1);
                return v;
            }

            std::span<std::type_index> get_types() {
                return component_info.get_span<std::type_index>();
            }

            Entity** get_backlink() {
                return backlink.get_ptr<Entity>();
            }

          private:
            std::size_t capacity;
            std::size_t rows;
            std::vector<std::byte*> components;
            multi_vector<std::size_t, std::type_index, Dtor*, MoveCtor*> component_info;
            std::size_t largest_size;
            multi_vector<Entity> backlink;

            bool moved = false;

            void resize(std::size_t new_capacity) {
                auto [sizes, dtor, move_ctor] = component_info.get_span<std::size_t, Dtor*, MoveCtor*>();

                for (std::size_t i = 0; i < sizes.size(); i++) {
                    auto* new_vec = reinterpret_cast<std::byte*>(::operator new(sizes[i] * new_capacity));

                    for (std::size_t r = 0; r < rows; r++) {
                        move_ctor[i](new_vec + r * sizes[i], components[i] + r * sizes[i]);
                        dtor[i](components[i] + r * sizes[i]);
                    }

                    ::operator delete(components[i]);
                    components[i] = new_vec;
                }

                capacity = new_capacity;
            };
        };
    }

    template <typename... Ts> struct Static;
    template <typename... Ts> struct WithRuntime;
    struct All;
    namespace __ {
        template <typename T> struct is_archetype {
            static constexpr bool value = false;
        };
        template <typename... Ts> struct is_archetype<Archetype<Ts...>> {
            static constexpr bool value = true;
        };

        template <typename T>
        concept is_archetype_v = is_archetype<T>::value;

        template <typename... Ts> struct query;
        template <typename T, typename... Ts> struct query<T, Ts...> {
            template <template <typename...> typename Set> using archetypes = Set<>;
            template <template <typename...> typename Set> using subset = Set<T, Ts...>;
            static constexpr bool runtime = true;
        };
        template <typename... Arches, typename... Ts> struct query<Static<Arches...>, Ts...> {
            template <template <typename...> typename Set> using archetypes = Set<Arches...>;
            template <template <typename...> typename Set> using subset = Set<Ts...>;
            static constexpr bool runtime = false;
        };
        template <typename... Arches, typename... Ts> struct query<WithRuntime<Arches...>, Ts...> {
            template <template <typename...> typename Set> using archetypes = Set<Arches...>;
            template <template <typename...> typename Set> using subset = Set<Ts...>;
            static constexpr bool runtime = true;
        };
        template <typename... Ts> struct query<All, Ts...> {
            template <template <typename...> typename Set> using archetypes = Set<>;
            template <template <typename...> typename Set> using subset = Set<Ts...>;
            static constexpr bool runtime = true;
        };
        template <typename Arch, typename... Ts>
            requires is_archetype_v<Arch>
        struct query<Arch, Ts...> {
            template <template <typename...> typename Set> using archetypes = Set<Arch>;
            template <template <typename...> typename Set> using subset = Set<Ts...>;
            static constexpr bool runtime = false;
        };
    }

    enum SystemRunnerOpts : uint8_t {
        WithIDs = 1 << 0,
        MarkDirty = 1 << 1,
        OnlyDirty = 1 << 2,
        StrictOnlyDirty = 1 << 3,
        KeepDirty = 1 << 4,
        SafeInsert = 1 << 5,
    };

    constexpr SystemRunnerOpts operator|(SystemRunnerOpts a, SystemRunnerOpts b) {
        return static_cast<SystemRunnerOpts>(static_cast<std::underlying_type_t<SystemRunnerOpts>>(a) |
                                             static_cast<std::underlying_type_t<SystemRunnerOpts>>(b));
    }

    constexpr SystemRunnerOpts operator&(SystemRunnerOpts a, SystemRunnerOpts b) {
        return static_cast<SystemRunnerOpts>(static_cast<std::underlying_type_t<SystemRunnerOpts>>(a) &
                                             static_cast<std::underlying_type_t<SystemRunnerOpts>>(b));
    }

    template <__::is_archetype_v... Archetypes>
        requires typeset::different_sets_v<Archetypes...>
    struct build {
      private:
        template <std::size_t IX> struct index {
            using T = typeset::nth_t<IX, Archetypes...>;
        };

        template <typename... Qs> struct setup_query {
            using __impl = __::query<Qs...>;
            template <template <typename...> typename Set> using __archetypes = __impl::template archetypes<Set>;
            template <template <typename...> typename Set> using __subset = __impl::template subset<Set>;

            template <template <typename...> typename Set>
            using archetypes = std::conditional_t<typeset::set_size_v<__archetypes<type_set>> == 0, Set<Archetypes...>,
                                                  __archetypes<Set>>;

            template <template <typename...> typename Set>
            using subset = std::conditional_t<
                typeset::set_size_v<archetypes<type_set>> == 1 && typeset::set_size_v<__subset<type_set>> == 0,
                typeset::change_set_carrier_t<typeset::set_nth_t<0, archetypes<type_set>>, Set>, __subset<Set>>;

            static constexpr bool runtime = __impl::runtime;
        };

        template <typename F> void visit(F&& f, std::size_t archetype_id) {
            if (is_runtime_archetype(archetype_id)) {
                f(get_runtime_archetype(archetype_id));
                return;
            }

            __::for_each_index(std::index_sequence_for<Archetypes...>{}, [&](auto I) {
                constexpr auto Ix = I.value;
                if (Ix != archetype_id) return false;

                f(*reinterpret_cast<index<Ix>::T*>(archetypes[Ix]));
                return true;
            });
        }

        template <typename... Components> struct in_archetypes {
            template <typename... Arches>
            static consteval std::pair<std::array<bool, sizeof...(Archetypes)>, std::size_t> _get() {
                std::array<bool, sizeof...(Archetypes)> matches = {(typeset::void_f<Archetypes>::f(), 0)...};
                ((matches[to_index<Arches>::value] = typeset::is_subset_v<type_set<Components...>, Arches>), ...);
                std::size_t count = (0 + ... + (matches[to_index<Arches>::value] ? 1 : 0));

                return {matches, count};
            }

            template <typename... Arches> static consteval auto get() {
                constexpr auto out = _get<Arches...>();
                constexpr auto matches = out.first;
                constexpr auto size = out.second;

                std::array<std::size_t, size> res;
                std::size_t writer_ix = 0;

                ((matches[to_index<Arches>::value] ? (res[writer_ix++] = to_index<Arches>::value, 0) : 0), ...);

                return res;
            };

            template <typename... Arches> static constexpr auto value = get<Arches...>();
        };

        template <std::size_t A, typename... Extra> struct possible_to_extend {
            template <typename U, typename Y> struct __impl;
            template <template <typename...> typename U, std::size_t... Res>
            struct __impl<U<>, std::index_sequence<Res...>> {
                using T = std::index_sequence<Res...>;
            };
            template <template <typename...> typename U, typename E, typename... Extras, std::size_t... Res>
            struct __impl<U<E, Extras...>, std::index_sequence<Res...>> {
                static consteval std::size_t get() {
                    using DiffT = typeset::set_difference_t<typename index<A>::T, type_set<E, Extras...>, Archetype>;
                    if constexpr ((typeset::is_same_set_v<DiffT, Archetypes> || ...)) {
                        return to_index<DiffT>::value;
                    }

                    return null_id;
                }

                static constexpr std::size_t value = get();
                using T =
                    std::conditional_t<value != null_id,
                                       typename __impl<type_set<Extras...>, std::index_sequence<Res..., value>>::T,
                                       typename __impl<type_set<Extras...>, std::index_sequence<Res...>>::T>;
            };

            using T = __impl<type_set<Extra...>, std::index_sequence<>>::T;
        };

        template <std::size_t Target, std::size_t Current, typename... Extra, typename... Packs>
        void static_extend_into(Entity ent, Packs&&... packs) {
            using TargetArch = index<Target>::T;
            using CurrentArch = index<Current>::T;

            auto* target_arch = reinterpret_cast<TargetArch*>(archetypes[Target]);
            auto* current_arch = reinterpret_cast<CurrentArch*>(archetypes[Current]);

            auto dest = target_arch->unsafe_push_entity(entities, ent);
            [&]<typename... SrcComps, typename... TargetComps>(std::in_place_type_t<Archetype<SrcComps...>>,
                                                               std::in_place_type_t<Archetype<TargetComps...>>) {
                auto src = current_arch->template get_subset<SrcComps...>();

                std::size_t src_row = entities[ent].row;

                __::for_each_index(std::index_sequence_for<SrcComps...>{}, [&](auto I) {
                    constexpr auto Ix = I.value;
                    using SrcTag = typeset::nth_t<Ix, SrcComps...>;
                    using SrcT = get_type_t<SrcTag>;
                    constexpr auto DestIx =
                        typeset::find_first_v<typeset::is_same_to<SrcTag>::template apply, TargetComps...>;

                    if constexpr (std::is_move_constructible_v<SrcT>) {
                        std::construct_at(std::get<DestIx>(dest),
                                          std::move(reinterpret_cast<SrcT*>(*src[Ix + 1])[src_row]));
                    } else {
                        std::construct_at(std::get<DestIx>(dest), reinterpret_cast<SrcT*>(*src[Ix + 1])[src_row]);
                    }

                    std::destroy_at(&reinterpret_cast<SrcT*>(*src[Ix + 1])[src_row]);

                    return false;
                });

                __::for_each_index(std::index_sequence_for<Extra...>{}, [&](auto I) {
                    constexpr auto Ix = I.value;
                    using TTag = typeset::nth_t<Ix, Extra...>;
                    using T = get_type_t<TTag>;
                    constexpr auto DestIx =
                        typeset::find_first_v<typeset::is_same_to<TTag>::template apply, TargetComps...>;

                    if constexpr (typeset::is_pack_v<typeset::nth_t<Ix, Packs...>> &&
                                  !std::is_same_v<T, typeset::nth_t<Ix, Packs...>>) {
                        typeset::with_pack(std::get<Ix>(std::forward_as_tuple(std::forward<Packs>(packs)...)),
                                           [&](auto... args) { std::construct_at(std::get<DestIx>(dest), args...); });
                    } else {
                        std::construct_at(std::get<DestIx>(dest),
                                          std::get<Ix>(std::forward_as_tuple(std::forward<Packs>(packs)...)));
                    }

                    return false;
                });
            }(std::in_place_type_t<CurrentArch>{}, std::in_place_type_t<TargetArch>{});

            current_arch->remove(entities, entities[ent].row);
            entities[ent].archetype_id = Target;
        }

        runtime::Archetype& get_runtime_archetype(std::size_t archetype_id) {
            return runtime_archetypes[archetype_id - sizeof...(Archetypes)];
        }

        std::vector<std::type_index> archetype_types(std::size_t arch_id) {
            assert(arch_id < sizeof...(Archetypes) + runtime_archetypes.size());

            std::vector<std::type_index> res;
            if (arch_id < sizeof...(Archetypes)) {
                __::for_each_index(std::index_sequence_for<Archetypes...>{}, [&](auto I) {
                    constexpr auto Ix = I.value;
                    if (Ix != arch_id) return false;

                    [&]<typename... Ts>(std::in_place_type_t<Archetype<Ts...>>) {
                        res.reserve(sizeof...(Ts));
                        (res.emplace_back(typeid(Ts)), ...);
                    }(std::in_place_type_t<typeset::nth_t<Ix, Archetypes...>>{});

                    return true;
                });

                return res;
            }

            auto& arch = runtime_archetypes[arch_id - sizeof...(Archetypes)];
            auto ts = arch.get_types();
            res.insert(res.end(), ts.begin(), ts.end());

            return res;
        }

        bool moved = false;
        std::vector<ArchetypeLink> entities;
        std::array<void*, sizeof...(Archetypes)> archetypes;

        std::vector<runtime::Archetype> runtime_archetypes;
        std::unordered_map<std::type_index, std::vector<std::size_t>> components_of_runtime_archetype;

      public:
        template <typename Arch>
            requires(typeset::is_same_set_v<Arch, Archetypes> || ...)
        struct to_index {
            static constexpr std::size_t value =
                typeset::find_first_v<typeset::curry2<typeset::is_same_set, Arch>::template apply, Archetypes...>;
        };

        build() {
            __::with_index_sequence(std::index_sequence_for<Archetypes...>{},
                                    [&](auto... Is) { ((archetypes[Is] = new index<Is>::T{}), ...); });
        };

        build(const build&) = delete;
        build& operator=(const build&) = delete;

        build(build&& b) noexcept
            : entities(std::move(b.entities)), archetypes(std::move(b.archetypes)),
              runtime_archetypes(std::move(b.runtime_archetypes)) {
            b.moved = true;
        };
        build& operator=(build&& b) noexcept {
            if (this == &b) return *this;

            entities = std::move(b.entities);

            __::with_index_sequence(std::index_sequence_for<Archetypes...>{}, [&](auto... Is) {
                ((delete reinterpret_cast<index<Is>::T*>(archetypes[Is])), ...);
            });
            archetypes = b.archetypes;
            runtime_archetypes = std::move(b.runtime_archetypes);

            b.moved = true;
            return *this;
        };

        ~build() {
            if (moved) return;

            __::with_index_sequence(std::make_index_sequence<sizeof...(Archetypes)>(), [&](auto... Is) {
                ((delete (reinterpret_cast<index<Is>::T*>(archetypes[Is]))), ...);
            });
        }

        inline bool is_runtime_archetype(std::size_t archetype_id) {
            return archetype_id >= sizeof...(Archetypes);
        }

        inline std::size_t get_archetype(Entity ent) {
            return entities[ent].archetype_id;
        }

        std::optional<std::size_t> archetype_exists(const std::span<std::type_index>& types) {
            std::size_t archetype_id;

            std::size_t t_size = types.size();
            bool found = ([&]<typename... Ts>(std::in_place_type_t<Archetype<Ts...>>) {
                if (sizeof...(Ts) != t_size) return false;

                bool ok = true;
                __::for_each_index(std::index_sequence_for<Ts...>{}, [&](auto I) {
                    constexpr auto Ix = I.value;

                    ok = ok && types[Ix] == typeid(typeset::nth_t<Ix, Ts...>);
                    return !ok;
                });

                if (ok) archetype_id = to_index<Archetype<Ts...>>::value;

                return ok;
            }(std::in_place_type_t<Archetypes>{}) ||
                          ...);

            if (found) {
                return archetype_id;
            }

            for (auto [arch_ix, ra] : runtime_archetypes | std::views::enumerate) {
                auto ts = ra.get_types();

                if (ts.size() != t_size) continue;

                bool ok = true;
                for (std::size_t i = 0; i < ts.size(); i++) {
                    ok = ok && ts[i] == types[i];
                    if (!ok) break;
                }

                if (ok) return std::size_t(arch_ix) + sizeof...(Archetypes);
            }

            return std::nullopt;
        }

        std::size_t add_archetype(runtime::Archetype&& arch) {
            if (auto exists = archetype_exists(arch.get_types()); exists) return *exists;

            auto arch_ix = runtime_archetypes.size();
            auto arch_id = sizeof...(Archetypes) + runtime_archetypes.size();
            runtime_archetypes.emplace_back(std::move(arch));

            auto types = runtime_archetypes[arch_ix].get_types();
            for (const auto& t : types) {
                auto it = components_of_runtime_archetype.find(t);
                if (it != components_of_runtime_archetype.end()) {
                    it->second.emplace_back(arch_id);
                } else {
                    components_of_runtime_archetype.emplace(t, std::vector<std::size_t>{arch_id});
                }
            }

            return arch_id;
        }

        template <typeset::unique_v... Ts> std::size_t new_archetype() {
            {
                std::type_index ts[] = {typeid(Ts)...};
                if (auto exists = archetype_exists({ts, sizeof...(Ts)}); exists) return *exists;
            }

            multi_vector<std::size_t, std::type_index, runtime::Dtor*, runtime::MoveCtor*> mv(sizeof...(Ts));
            (mv.emplace_back(sizeof(Ts), typeid(Ts), &runtime::generic_dtor<Ts>, &runtime::generic_move_ctor<Ts>), ...);

            auto arch_ix = runtime_archetypes.size();
            auto arch_id = sizeof...(Archetypes) + runtime_archetypes.size();
            runtime_archetypes.emplace_back(std::move(mv));

            auto types = runtime_archetypes[arch_ix].get_types();
            for (const auto& t : types) {
                auto it = components_of_runtime_archetype.find(t);
                if (it != components_of_runtime_archetype.end()) {
                    it->second.emplace_back(arch_id);
                } else {
                    components_of_runtime_archetype.emplace(t, std::vector<std::size_t>{arch_id});
                }
            }

            return arch_id;
        }

        template <typename... Subset> bool is_dirty(Entity ent) {
            bool ret = false;

            if constexpr (sizeof...(Subset) != 0) {
                constexpr auto in_arches = in_archetypes<Subset...>::template value<Archetypes...>;

                __::for_each_index(std::make_index_sequence<in_arches.size()>{}, [&](auto I) {
                    constexpr auto Ix = I.value;
                    if (entities[ent].archetype_id != in_arches[Ix]) return false;

                    auto* arch =
                        reinterpret_cast<typeset::nth_t<in_arches[Ix], Archetypes...>*>(archetypes[in_arches[Ix]]);
                    ret = arch->template get_dirty<Subset...>(entities[ent].row);
                    return true;
                });

                return ret;
            }

            visit(
                [&](auto& arch) {
                    using Arch = std::decay_t<decltype(arch)>;
                    if constexpr (std::is_same_v<Arch, runtime::Archetype>) {
                        // TODO: runtime archetype dirty marking
                    } else {
                        ret = arch.get_dirty(entities[ent].row);
                    }
                },
                entities[ent].archetype_id);
            return ret;
        }

        Entity new_entity() {
            entities.emplace_back(null_id, null_id);

            return entities.size() - 1;
        };

        inline std::size_t entity_archetype(Entity ent) const {
            return entities[ent].archetype_id;
        }

        template <typename... Extra, typename... Packs> void static_extend(Entity ent, Packs&&... packs) {
            constexpr auto possible = in_archetypes<Extra...>::template value<Archetypes...>;

            if (entities[ent].archetype_id == null_id || entities[ent].row == null_id)
                assert(false && "static_extend called on uninitialized entity; PS: add exceptions");

            __::for_each_index(std::make_index_sequence<possible.size()>{}, [&](auto TI) -> bool {
                constexpr auto Target = TI.value;

                if constexpr (typeset::set_size_v<typename index<possible[Target]>::T> == sizeof...(Packs) &&
                              sizeof...(Packs) == sizeof...(Extra)) {
                    if (entities[ent].archetype_id == possible[Target]) {
                        static_set_entity<typename index<possible[Target]>::T>(ent, std::forward<Packs>(packs)...);
                        return true;
                    }
                }

                return __::for_each_index(typename possible_to_extend<possible[Target], Extra...>::T{},
                                          [&](auto PossibleI) -> bool {
                                              constexpr auto Possible = PossibleI.value;

                                              if (entities[ent].archetype_id == Possible) {
                                                  static_extend_into<possible[Target], Possible, Extra...>(
                                                      ent, std::forward<Packs>(packs)...);
                                                  return true;
                                              }

                                              return false;
                                          });
            });
        }

        void dynamic_extend(Entity ent,
                            multi_vector<std::size_t, std::type_index, runtime::Dtor*, runtime::MoveCtor*>&& extra,
                            void* args[], std::size_t nargs) {
            if (entities[ent].archetype_id == null_id || entities[ent].row == null_id)
                assert(false && "extend called on uninitialized entity; PS: add exceptions");

            auto ts = archetype_types(entities[ent].archetype_id);
            auto extra_ts = extra.get_span<std::type_index>();

            std::size_t orig_size = ts.size();
            for (std::size_t e = 0; e < extra_ts.size(); e++) {
                bool exists = false;
                for (std::size_t i = 0; i < orig_size; i++) {
                    if (ts[i] == extra_ts[e]) {
                        exists = true;
                        break;
                    }
                }

                if (!exists) ts.emplace_back(extra_ts[e]);
            }

            std::size_t orig_arch_id = entities[ent].archetype_id;
            std::size_t ent_row = entities[ent].row;
            std::size_t new_arch_id{static_cast<size_t>(-1)};
            std::vector<void*> new_args{};

            if (auto exists = archetype_exists({ts.data(), ts.size()}); exists) {
                if (orig_arch_id == *exists) {
                    set_entity(*exists, ent, args, nargs);
                    return;
                }

                new_arch_id = *exists;
            }

            if (is_runtime_archetype(orig_arch_id)) {
                auto& orig_arch = get_runtime_archetype(orig_arch_id);
                if (new_arch_id == null_id) {
                    auto new_ts = orig_arch.extend(extra);
                    new_arch_id = add_archetype(runtime::Archetype{std::move(new_ts)});
                }

                new_args = orig_arch.get_row(ent_row);
            } else {
                __::for_each_index(std::index_sequence_for<Archetypes...>{}, [&](auto I) {
                    constexpr auto Ix = I.value;
                    if (Ix != orig_arch_id) return false;

                    auto orig_arch = reinterpret_cast<index<Ix>::T*>(archetypes[Ix]);
                    if (new_arch_id == null_id) {
                        auto new_ts = reinterpret_cast<index<Ix>::T*>(archetypes[Ix])->extend(extra);
                        new_arch_id = add_archetype(runtime::Archetype{std::move(new_ts)});
                    }

                    new_args = orig_arch->get_row_vec(ent_row);
                    return true;
                });
            }
            new_args.insert(new_args.end(), args, args + nargs);

            entities[ent].archetype_id = null_id;
            entities[ent].row = null_id;
            dynamic_set_entity(new_arch_id, ent, new_args.data(), new_args.size());

            auto ent_info = entities[ent];

            entities[ent].archetype_id = orig_arch_id;
            entities[ent].row = ent_row;
            remove(ent);

            entities[ent] = ent_info;
        }

        template <typename... Extra, typename... Args> void extend(Entity ent, Args&&... args) {
            if (entities[ent].archetype_id == null_id || entities[ent].row == null_id)
                assert(false && "static_extend called on uninitialized entity; PS: add exceptions");

            multi_vector<std::size_t, std::type_index, runtime::Dtor*, runtime::MoveCtor*> mv;
            (mv.emplace_back(sizeof(Extra), typeid(Extra), &runtime::generic_dtor<Extra>,
                             &runtime::generic_move_ctor<Extra>),
             ...);

            void* args_arr[] = {&args...};
            dynamic_extend(ent, std::move(mv), args_arr, sizeof...(Args));
        }

        template <typename Arch, typename... Packs> void static_set_entity(Entity entity, Packs&&... packs) {
            constexpr auto archetype_id = to_index<Arch>::value;

            auto& e_link = entities[entity];
            if (archetype_id != e_link.archetype_id && e_link.archetype_id != null_id && e_link.row != null_id) {
                remove(entity);
            }

            e_link.archetype_id = archetype_id;

            auto* archetype = reinterpret_cast<Arch*>(archetypes[archetype_id]);
            if (e_link.row != null_id) {
                archetype->emplace_at(e_link.row, std::forward<Packs>(packs)...);
            } else {
                archetype->new_entity(entities, entity, std::forward<Packs>(packs)...);
            }
        }

        void dynamic_set_entity(std::size_t archetype_id, Entity ent, void* args[], std::size_t nargs) {
            auto& e_link = entities[ent];
            if (archetype_id != e_link.archetype_id && e_link.archetype_id != null_id && e_link.row != null_id) {
                remove(ent);
            }

            e_link.archetype_id = archetype_id;
            visit(
                [&](auto& archetype) {
                    using Arch = std::decay_t<decltype(archetype)>;
                    if constexpr (std::is_same_v<Arch, runtime::Archetype>) {
                        assert(archetype.get_types().size() == nargs);
                    } else {
                        assert(nargs == typeset::set_size_v<Arch>);
                    }

                    if (e_link.row != null_id) {
                        archetype.emplace_at(e_link.row, args, nargs);
                    } else {
                        archetype.new_entity(entities, ent, args, nargs);
                    }
                },
                archetype_id);
        }

        template <typename... Args> void set_entity(std::size_t archetype_id, Entity ent, Args&&... args) {
            void* args_arr[] = {&args...};
            dynamic_set_entity(archetype_id, ent, args_arr, sizeof...(Args));
        }

        template <typename Arch, typename... Packs>
            requires __::is_archetype_v<Arch> && (typeset::is_same_set_v<Arch, Archetypes> || ...)
        Entity static_emplace_entity(Packs&&... packs) {
            auto ent = new_entity();
            static_set_entity<Arch>(ent, std::forward<Packs>(packs)...);

            return ent;
        }

        Entity dynamic_emplace_entity(std::size_t archetype_id, void* args[], std::size_t nargs) {
            auto ent = new_entity();
            dynamic_set_entity(archetype_id, ent, args, nargs);

            return ent;
        }

        template <typename... Args> Entity emplace_entity(std::size_t archetype_id, Args&&... args) {
            void* args_arr[] = {&args...};
            return dynamic_emplace_entity(archetype_id, args_arr, sizeof...(Args));
        }

        void remove(Entity entity) {
            if (entity > entities.size() || entities[entity].archetype_id == null_id || entities[entity].row == null_id)
                return;

            auto& e_link = entities[entity];

            visit([&](auto& archetype) { archetype.remove(entities, e_link.row); }, e_link.archetype_id);

            e_link.archetype_id = null_id;
            e_link.row = null_id;
        }

        template <typename... Qs> decltype(auto) get(Entity ent) {
            using query = setup_query<Qs...>;
            using arches = query::template archetypes<type_set>;
            using return_t = query::template subset<typeset::ref_tuple_t>;
            constexpr auto in_arches = query::template archetypes<typeset::curry2<
                typeset::value_wrapper, typename query::template subset<in_archetypes>>::template apply>::value;
            static_assert(in_arches.size() == typeset::set_size_v<arches>);

            auto ent_arch_id = entities[ent].archetype_id;
            std::optional<return_t> ret = std::nullopt;
            __::for_each_index(std::make_index_sequence<in_arches.size()>{}, [&](auto I) {
                constexpr auto Ix = I.value;
                if (ent_arch_id != in_arches[Ix]) return false;
                using Arch = index<in_arches[Ix]>::T;

                auto* arch = reinterpret_cast<Arch*>(archetypes[in_arches[Ix]]);

                [&]<typename... Ts>(std::in_place_type_t<type_set<Ts...>>) {
                    ret.emplace(arch->template get<Ts...>(entities[ent].row));
                }(std::in_place_type_t<typename query::template subset<type_set>>{});
                return true;
            });

            if (ret) return ret;
            if (!query::runtime || !is_runtime_archetype(ent_arch_id)) return ret;

            auto& arch = get_runtime_archetype(ent_arch_id);
            auto ts = arch.get_types();
            [&]<typename... Subset>(std::in_place_type_t<type_set<Subset...>>) {
                std::type_index sub[] = {typeid(Subset)...};
                std::span<std::type_index> sub_ts{sub, sizeof...(Subset)};
                if (!typeset::subset({sub, sizeof...(Subset)}, ts)) return;

                auto row = arch.get_row(entities[ent].row, sub_ts);
                __::with_index_sequence(std::index_sequence_for<Subset...>{},
                                        [&](auto... Is) { ret.emplace(*reinterpret_cast<Subset*>(row[Is])...); });
            }(std::in_place_type_t<typename query::template subset<type_set>>{});

            return ret;
        }

        // TODO: feature parity with `get`
        std::optional<std::vector<void*>> dynamic_get(Entity ent, std::size_t expected_archetype,
                                                      std::span<std::type_index> subset = {(std::type_index*)nullptr,
                                                                                           0}) {
            if (entities[ent].archetype_id != expected_archetype) return std::nullopt;

            std::vector<void*> ret;
            visit(
                [&](auto& archetype) {
                    using Arch = std::decay_t<decltype(archetype)>;
                    if constexpr (std::is_same_v<Arch, runtime::Archetype>) {
                        ret = archetype.get_row(entities[ent].row, subset);
                    } else {
                        ret = archetype.get_row_vec(entities[ent].row, subset);
                    }
                },
                expected_archetype);

            return ret;
        }

        template <typename F>
        void find_dead(F&& f, std::size_t limit = std::numeric_limits<std::size_t>::max(), std::size_t start_at = 0) {
            for (std::size_t i = start_at; i < std::min(start_at + limit, entities.size()); i++) {
                if (entities[i].archetype_id == null_id && entities[i].row == null_id) {
                    f(i);
                }
            }
        }

        template <typename... Ts> class System;

        template <typename... Arches, typename... Subset> class System<type_set<Arches...>, Subset...> {
          public:
            static consteval decltype(auto) get_dirty_impl() {
                std::array<bool, sizeof...(Subset)> mask{!std::is_const_v<Subset>...};

                std::size_t size = __::with_index_sequence(std::index_sequence_for<Subset...>{}, [&](auto... Is) {
                    return (0uz + ... + (mask[Is] ? 1uz : 0uz));
                });

                return std::pair{mask, size};
            }

            static consteval decltype(auto) get_dirty_impl2() {
                constexpr auto mask = get_dirty_impl();

                std::array<std::size_t, mask.second> dirty;
                std::size_t writer_ix = 0;
                __::with_index_sequence(std::index_sequence_for<Subset...>{}, [&](auto... Is) {
                    ((mask.first[Is] ? (dirty[writer_ix++] = Is, 0) : 0), ...);
                });

                return dirty;
            }

            template <auto dirty> static consteval decltype(auto) get_dirty() {
                return __::with_index_sequence(std::make_index_sequence<dirty.size()>{}, [&](auto... Is) {
                    return type_set<typeset::nth_t<dirty[Is], Subset...>...>{};
                });
            }

            // these components will get marked as "dirty" in the runner on flag MarkDirty
            static constexpr auto dirty_components_ix = get_dirty_impl2();
            using dirty_components = decltype(get_dirty<dirty_components_ix>());

            static constexpr auto archetypes = in_archetypes<std::remove_const_t<Subset>...>::template value<Arches...>;
            std::array<void*, archetypes.size()> archetype_pointers;
            std::vector<runtime::Archetype*> runtime_archetypes{};

            std::array<std::vector<void**>, sizeof...(Subset)> data{};
            std::vector<std::array<uint64_t**, sizeof...(Subset)>> dirty_indexes;
            std::vector<std::size_t*> data_sizes{};
            std::vector<Entity**> data_backlinks{};

            template <std::size_t IX> void setup_archetype(const std::array<void*, sizeof...(Archetypes)>& arches) {
                constexpr auto arch_id = archetypes[IX];
                using archetype = index<arch_id>::T;
                archetype_pointers[IX] = arches[arch_id];
                archetype* x = (archetype*)arches[arch_id];
                auto subset = x->template get_subset<std::remove_const_t<Subset>...>();

                for (std::size_t i = 0; i < data.size(); i++) {
                    data[i].emplace_back(subset[i + 1]);
                }

                data_sizes.emplace_back((std::size_t*)subset[0]);
                data_backlinks.emplace_back(x->get_backlink());
                dirty_indexes.emplace_back(x->template get_dirty_ptrs<std::remove_const_t<Subset>...>());
            }

            void setup_runtime_archetypes(
                std::vector<runtime::Archetype>& runtime_arches,
                const std::unordered_map<std::type_index, std::vector<std::size_t>>& runtime_arches_table) {
                std::array<std::type_index, sizeof...(Subset)> needed = {typeid(Subset)...};

                std::vector<std::size_t> possible = runtime_arches_table.at(needed[0]);

                for (std::size_t i = 1; i < needed.size(); i++) {
                    const auto& opts = runtime_arches_table.at(needed[i]);

                    std::erase_if(possible, [&](std::size_t id) -> bool {
                        for (const auto& opt : opts) {
                            if (opt == id) return false;
                        }

                        return true;
                    });

                    if (possible.empty()) break;
                }

                for (const auto& arch_id : possible) {
                    runtime_archetypes.emplace_back(&runtime_arches[arch_id - sizeof...(Archetypes)]);
                    auto& archetype = runtime_arches[arch_id - sizeof...(Archetypes)];
                    std::type_index subset_ts[] = {typeid(Subset)...};
                    auto subset = archetype.get_subset(std::span{subset_ts, sizeof...(Subset)});

                    for (std::size_t i = 0; i < data.size(); i++) {
                        data[i].emplace_back(subset[i + 1]);
                    }
                    data_sizes.emplace_back((std::size_t*)subset[0]);
                    data_backlinks.emplace_back(archetype.get_backlink());
                }
            }

            System(const std::array<void*, sizeof...(Archetypes)>& arches,
                   const std::vector<runtime::Archetype>& runtime_arches,
                   const std::unordered_map<std::type_index, std::vector<std::size_t>>& runtime_arches_table) {
                dirty_indexes.reserve(sizeof...(Subset));
                data_sizes.reserve(archetypes.size());
                data_backlinks.reserve(archetypes.size());

                __::with_index_sequence(std::make_index_sequence<archetypes.size()>(), [&](auto... Is) {
                    ((data[Is].reserve(archetypes.size()), setup_archetype<Is>(arches)), ...);
                });

                if (runtime_arches_table.size() != 0) {
                    setup_runtime_archetypes(const_cast<std::vector<runtime::Archetype>&>(runtime_arches),
                                             runtime_arches_table);
                }
            }

          public:
            friend build;

            template <SystemRunnerOpts opts = static_cast<SystemRunnerOpts>(0), typename F> void run(F&& f) {
                constexpr auto StrictOnlyDirtyFlag = (opts & StrictOnlyDirty) != 0;
                constexpr auto OnlyDirtyFlag = ((opts & OnlyDirty) != 0) || StrictOnlyDirtyFlag;
                constexpr auto MarkDirtyFlag = (opts & MarkDirty) != 0;
                constexpr auto KeepDirtyFlag = (opts & KeepDirty) != 0;

                std::vector<std::size_t> dirty_end_ranges;
                if constexpr (MarkDirtyFlag && !OnlyDirtyFlag) dirty_end_ranges.reserve(data_sizes.size());

                __::with_index_sequence(std::index_sequence_for<Subset...>{}, [&](auto... SubSeq) {
                    __::for_each_index(std::make_index_sequence<archetypes.size()>{}, [&](auto ArchI) {
                        constexpr auto ArchIx = ArchI.value;
                        auto f_extra = [&]<typename... Extra>(std::size_t i, Extra&&... extra) {
                            if constexpr ((opts & SafeInsert) != 0) {
                                std::tuple<std::remove_reference_t<Subset>...> copy{reinterpret_cast<
                                    get_type_t<typeset::nth_t<SubSeq, std::remove_reference_t<Subset>...>>*>(
                                    *data[SubSeq][ArchIx])[i]...};

                                f(std::forward<Extra>(extra)..., std::get<SubSeq>(copy)...);

                                ((reinterpret_cast<
                                      get_type_t<typeset::nth_t<SubSeq, std::remove_reference_t<Subset>...>>*>(
                                      *data[SubSeq][ArchIx])[i] = std::get<SubSeq>(copy)),
                                 ...);
                            } else {
                                f(std::forward<Extra>(extra)...,
                                  (reinterpret_cast<
                                      get_type_t<typeset::nth_t<SubSeq, std::remove_reference_t<Subset>...>>*>(
                                      *data[SubSeq][ArchIx])[i])...);
                            }
                        };
                        auto f_complete = [&](std::size_t i) {
                            if constexpr ((opts & WithIDs) != 0) {
                                f_extra(i, std::as_const((*data_backlinks[ArchIx])[i]));
                            } else {
                                f_extra(i);
                            }
                        };

                        if constexpr (OnlyDirtyFlag) {
                            auto word_count = *data_sizes[ArchIx] / 64 + (*data_sizes[ArchIx] % 64 != 0 ? 1 : 0);
                            std::array<uint64_t**, sizeof...(Subset)>& dirty_ixs = dirty_indexes[ArchIx];
                            for (std::size_t w = 0; w < word_count; w++) {
                                uint64_t mask_union = 0ULL;
                                for (std::size_t i = 0; i < sizeof...(Subset); i++) {
                                    if constexpr (StrictOnlyDirtyFlag) mask_union &= (*dirty_ixs[i])[w];
                                    else mask_union |= (*dirty_ixs[i])[w];

                                    if constexpr (!KeepDirtyFlag && !StrictOnlyDirtyFlag) (*dirty_ixs[i])[w] = 0ULL;
                                }

                                uint64_t index_mask = ~0ULL;
                                while (true) {
                                    std::size_t ix = static_cast<std::size_t>(std::countr_zero(mask_union));
                                    if (ix == 64) break;

                                    f_complete(ix + w * 64);

                                    index_mask &= ~0ULL ^ (1ULL << ix);

                                    if (ix == 63) break;
                                    mask_union &= (~0ULL << (ix + 1));
                                }

                                if constexpr (!KeepDirtyFlag && StrictOnlyDirtyFlag) {
                                    for (std::size_t i = 0; i < sizeof...(Subset); i++) {
                                        (*dirty_ixs[i])[w] &= index_mask;
                                    }
                                } else if constexpr (MarkDirtyFlag) {
                                    for (std::size_t i = 0; i < dirty_components_ix.size(); i++) {
                                        (*dirty_ixs[dirty_components_ix[i]])[w] |= ~index_mask;
                                    }
                                }
                            }
                        } else {
                            for (std::size_t i = 0; i < *data_sizes[ArchIx]; i++) {
                                f_complete(i);
                            }
                        }

                        if constexpr (MarkDirtyFlag && !OnlyDirtyFlag) {
                            dirty_end_ranges.emplace_back(*data_sizes[ArchIx]);
                        }

                        return false;
                    });

                    for (std::size_t a = 0; a < runtime_archetypes.size(); a++) {
                        for (std::size_t i = 0; i < *data_sizes[archetypes.size() + a]; i++) {
                            // TODO: add safe insert
                            if constexpr ((opts & WithIDs) != 0) {
                                f(std::as_const((*data_backlinks[archetypes.size() + a])[i]),
                                  (reinterpret_cast<
                                      get_type_t<typeset::nth_t<SubSeq, std::remove_reference_t<Subset>...>>*>(
                                      *data[SubSeq][archetypes.size() + a])[i])...);
                            } else {
                                f((reinterpret_cast<
                                    get_type_t<typeset::nth_t<SubSeq, std::remove_reference_t<Subset>...>>*>(
                                    *data[SubSeq][archetypes.size() + a])[i])...);
                            }
                        }
                        if constexpr (MarkDirtyFlag && !OnlyDirtyFlag)
                            dirty_end_ranges.emplace_back(*data_sizes[archetypes.size() + a]);
                    }
                });

                if (MarkDirtyFlag && !OnlyDirtyFlag) {
                    __::for_each_index(std::make_index_sequence<archetypes.size()>{}, [&](auto I) {
                        constexpr auto Ix = I.value;
                        constexpr auto ArchIx = archetypes[Ix];
                        using Arch = index<ArchIx>::T;

                        std::size_t range_end = dirty_end_ranges[Ix];
                        auto* arch = reinterpret_cast<Arch*>(archetype_pointers[ArchIx]);
                        [&]<typename... Ts>(type_set<Ts...>) {
                            arch->template mark_dirty<Ts...>(0, range_end);
                        }(dirty_components{});

                        return false;
                    });

                    // TODO: runtiem component dirty marking
                }
            }
        };

        template <typename... Qs> decltype(auto) make_system_query() {
            using query = setup_query<Qs...>;
            using arches = query::template archetypes<type_set>;
            using Sys = query::template subset<typeset::curry2<System, arches>::template apply>;

            static_assert(std::is_same_v<Sys, System<type_set<Archetype<int, char>>, int, char>>);

            if (query::runtime) {
                return Sys(archetypes, runtime_archetypes, components_of_runtime_archetype);
            } else {
                std::vector<runtime::Archetype> ra;
                std::unordered_map<std::type_index, std::vector<std::size_t>> m;
                return Sys(archetypes, ra, m);
            }
        };

        template <typename... Subset>
            requires(sizeof...(Subset) > 1 || !__::is_archetype_v<typeset::nth_t<0, Subset...>>)
        System<type_set<Archetypes...>, Subset...> make_static_system() {
            return System<type_set<Archetypes...>, Subset...>(archetypes, {}, {});
        }

        template <typename Arch>
            requires __::is_archetype_v<Arch>
        decltype(auto) make_static_system() {
            return [&]<typename... Ts>(std::in_place_type_t<Archetype<Ts...>>) {
                return System<type_set<Archetypes...>, Ts...>(archetypes, {}, {});
            }(std::in_place_type_t<Arch>{});
        }

        template <typename... Subset>
            requires(sizeof...(Subset) > 1 || !__::is_archetype_v<typeset::nth_t<0, Subset...>>)
        System<type_set<Archetypes...>, Subset...> make_system() {
            return System<type_set<Archetypes...>, Subset...>(archetypes, runtime_archetypes,
                                                              components_of_runtime_archetype);
        }

        template <typename Arch>
            requires __::is_archetype_v<Arch>
        decltype(auto) make_system() {
            return [&]<typename... Ts>(std::in_place_type_t<Archetype<Ts...>>) {
                return System<type_set<Archetypes...>, Ts...>(archetypes, runtime_archetypes,
                                                              components_of_runtime_archetype);
            }(std::in_place_type_t<Arch>{});
        }
    };
};
