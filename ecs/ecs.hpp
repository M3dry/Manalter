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
        // [0] - size
        multi_vector<Components...> components;
        std::array<std::vector<uint64_t>, sizeof...(Components)> dirty;
        std::vector<Entity> backlink;

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

        void mark_dirty(std::size_t row, bool state = true) {

        }

        void mark_dirty(std::size_t start, std::size_t end, bool state = true) {

        }

        bool get_dirty(std::size_t row) {

        }

        template <typename... Packs> void emplace_at(std::size_t row, Packs&&... packs) {
            components.emplace_at(row, std::forward<Packs>(packs)...);
        }

        void emplace_at(std::size_t row, void* args[], std::size_t nargs) {
            assert(nargs == sizeof...(Components));
            auto t = components.template get_ptr<Components...>(row);

            __::for_each_index(std::index_sequence_for<Components...>{}, [&](auto I) {
                constexpr auto Ix = I.value;
                using T = typeset::nth_t<Ix, Components...>;

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
            components.emplace_back(std::forward<Packs>(packs)...);

            link[entity].row = components.size() - 1;
            backlink.emplace_back(entity);

            mark_dirty(link[entity].row);
        };

        void new_entity(std::vector<ArchetypeLink>& link, Entity entity, void* args[], std::size_t nargs) {
            components.unsafe_push();
            emplace_at(components.size() - 1, args, nargs);

            link[entity].row = components.size() - 1;
            backlink.emplace_back(entity);

            mark_dirty(link[entity].row);
        }

        std::tuple<Components*...> unsafe_push_entity(std::vector<ArchetypeLink>& link, Entity entity) {
            auto t = components.unsafe_push();

            link[entity].row = components.size() - 1;
            backlink.emplace_back(entity);

            return t;
        }

        std::tuple<Components*...> get_row(std::size_t row) {
            return components.template get_ptr<Components...>(row);
        }

        std::vector<void*> get_row_vec(std::size_t row) {
            auto t = get_row(row);
            std::vector<void*> res{};
            res.reserve(sizeof...(Components));

            __::for_each_index(std::index_sequence_for<Components...>{}, [&](auto I) {
                constexpr auto Ix = I.value;

                res.emplace_back(std::get<Ix>(t));
                return false;
            });

            return res;
        }

        void remove(std::vector<ArchetypeLink>& link, std::size_t row) {
            // mark_dirty(row, get_dirty(components.size() - 1));

            components.swap(row, components.size() - 1);
            components.pop_back();

            backlink[row] = backlink[components.size()];
            backlink.pop_back();

            link[backlink[row]].row = row;
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

            std::vector<void*> get_row(std::size_t row) {
                std::vector<void*> res{};
                res.reserve(components.size());

                auto sizes = component_info.get_span<std::size_t>();
                for (std::size_t i = 0; i < components.size(); i++) {
                    res.emplace_back(reinterpret_cast<void*>(components[i] + row * sizes[i]));
                }

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

                backlink[row] = backlink[rows];
                backlink.pop_back();

                link[backlink[row]].row = row;
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

          private:
            std::size_t capacity;
            std::size_t rows;
            std::vector<std::byte*> components;
            multi_vector<std::size_t, std::type_index, Dtor*, MoveCtor*> component_info;
            std::size_t largest_size;
            std::vector<Entity> backlink;

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

    namespace __ {
        template <typename T> struct is_archetype {
            static constexpr bool value = false;
        };
        template <typename... Ts> struct is_archetype<Archetype<Ts...>> {
            static constexpr bool value = true;
        };

        template <typename T>
        concept is_archetype_v = is_archetype<T>::value;
    }

    template <__::is_archetype_v... Archetypes>
        requires typeset::different_sets_v<Archetypes...>
    struct build {
      private:
        template <std::size_t IX> struct index {
            using T = typeset::nth_t<IX, Archetypes...>;
        };

        template <typename... Components> struct in_archetypes {
            static consteval std::pair<std::array<bool, sizeof...(Archetypes)>, std::size_t> _get() {
                std::array<bool, sizeof...(Archetypes)> matches;
                ((matches[to_index<Archetypes>::value] = typeset::is_subset_v<type_set<Components...>, Archetypes>),
                 ...);
                std::size_t count = (0 + ... + (matches[to_index<Archetypes>::value] ? 1 : 0));

                return {matches, count};
            }

            static consteval auto get() {
                constexpr auto out = _get();
                constexpr auto matches = out.first;
                constexpr auto size = out.second;

                std::array<std::size_t, size> res;
                std::size_t writer_ix = 0;

                ((matches[to_index<Archetypes>::value] ? (res[writer_ix++] = to_index<Archetypes>::value, 0) : 0), ...);

                return res;
            };

            static constexpr auto value = get();
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

                    return size_t(-1);
                }

                static constexpr std::size_t value = get();
                using T =
                    std::conditional_t<value != size_t(-1),
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
                    using SrcT = typeset::nth_t<Ix, SrcComps...>;
                    constexpr auto DestIx =
                        typeset::find_first_v<typeset::is_same_to<SrcT>::template apply, TargetComps...>;

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
                    using T = typeset::nth_t<Ix, Extra...>;
                    constexpr auto DestIx =
                        typeset::find_first_v<typeset::is_same_to<T>::template apply, TargetComps...>;

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
            __::with_index_sequence(std::index_sequence_for<Archetypes...>{}, [&](auto... Is) {
                ((archetypes[Is] = new index<Is>::T{}), ...);
            });
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

            constexpr auto ix_seq = std::make_index_sequence<sizeof...(Archetypes)>();

            __::with_index_sequence(
                ix_seq, [&](auto... Is) { ((delete (reinterpret_cast<index<Is>::T*>(archetypes[Is]))), ...); });
        }

        bool is_runtime_archetype(std::size_t archetype_id) {
            return archetype_id >= sizeof...(Archetypes);
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

        Entity new_entity() {
            entities.emplace_back(std::size_t(-1), std::size_t(-1));

            return entities.size() - 1;
        };

        inline std::size_t entity_archetype(Entity ent) const {
            return entities[ent].archetype_id;
        }

        template <typename... Extra, typename... Packs> void static_extend(Entity ent, Packs&&... packs) {
            constexpr auto possible = in_archetypes<Extra...>::value;

            if (entities[ent].archetype_id == std::size_t(-1) || entities[ent].row == std::size_t(-1))
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
                                          [&](auto PI) -> bool {
                                              constexpr auto Possible = PI.value;

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
            if (entities[ent].archetype_id == std::size_t(-1) || entities[ent].row == std::size_t(-1))
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
                if (new_arch_id == size_t(-1)) {
                    auto new_ts = orig_arch.extend(extra);
                    new_arch_id = add_archetype(runtime::Archetype{std::move(new_ts)});
                }

                new_args = orig_arch.get_row(ent_row);
            } else {
                __::for_each_index(std::index_sequence_for<Archetypes...>{}, [&](auto I) {
                    constexpr auto Ix = I.value;
                    if (Ix != orig_arch_id) return false;

                    auto orig_arch = reinterpret_cast<index<Ix>::T*>(archetypes[Ix]);
                    if (new_arch_id == size_t(-1)) {
                        auto new_ts = reinterpret_cast<index<Ix>::T*>(archetypes[Ix])->extend(extra);
                        new_arch_id = add_archetype(runtime::Archetype{std::move(new_ts)});
                    }

                    new_args = orig_arch->get_row_vec(ent_row);
                    return true;
                });
            }
            new_args.insert(new_args.end(), args, args + nargs);

            entities[ent].archetype_id = size_t(-1);
            entities[ent].row = size_t(-1);
            dynamic_set_entity(new_arch_id, ent, new_args.data(), new_args.size());

            auto ent_info = entities[ent];

            entities[ent].archetype_id = orig_arch_id;
            entities[ent].row = ent_row;
            remove(ent);

            entities[ent] = ent_info;
        }

        template <typename... Extra, typename... Args> void extend(Entity ent, Args&&... args) {
            if (entities[ent].archetype_id == std::size_t(-1) || entities[ent].row == std::size_t(-1))
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
            if (archetype_id != e_link.archetype_id && e_link.archetype_id != size_t(-1) && e_link.row != size_t(-1)) {
                remove(entity);
            }

            e_link.archetype_id = archetype_id;

            auto* archetype = reinterpret_cast<Arch*>(archetypes[archetype_id]);
            if (e_link.row != size_t(-1)) {
                archetype->emplace_at(e_link.row, std::forward<Packs>(packs)...);
            } else {
                archetype->new_entity(entities, entity, std::forward<Packs>(packs)...);
            }
        }

        void dynamic_set_entity(std::size_t archetype_id, Entity ent, void* args[], std::size_t nargs) {
            auto& e_link = entities[ent];
            if (archetype_id != e_link.archetype_id && e_link.archetype_id != size_t(-1) && e_link.row != size_t(-1)) {
                remove(ent);
            }

            e_link.archetype_id = archetype_id;
            if (archetype_id < sizeof...(Archetypes)) {
                __::for_each_index(std::index_sequence_for<Archetypes...>{}, [&](auto I) {
                    constexpr auto Ix = I.value;

                    if (archetype_id != Ix) return false;

                    using ArchT = typeset::nth_t<Ix, Archetypes...>;
                    auto arch = reinterpret_cast<ArchT*>(archetypes[Ix]);

                    assert(nargs == typeset::set_size_v<ArchT>);

                    if (e_link.row != size_t(-1)) {
                        arch->emplace_at(e_link.row, args, nargs);
                    } else {
                        arch->new_entity(entities, ent, args, nargs);
                    }

                    return true;
                });

                return;
            }

            runtime::Archetype& arch = get_runtime_archetype(archetype_id);
            auto types = arch.get_types();
            assert(types.size() == nargs);

            if (e_link.row != size_t(-1)) {
                arch.emplace_at(e_link.row, args, nargs);
            } else {
                arch.new_entity(entities, ent, args, nargs);
            }
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
            if (entity > entities.size() || entities[entity].archetype_id == size_t(-1) ||
                entities[entity].row == size_t(-1))
                return;
            auto& e_link = entities[entity];

            __::with_index_sequence(std::index_sequence_for<Archetypes...>{}, [&](auto... Is) {
                bool found = false;
                ((!found ? (
                               [&](std::size_t _) {
                                   if (Is == e_link.archetype_id) {
                                       auto* arch = reinterpret_cast<index<Is>::T*>(archetypes[Is]);
                                       arch->remove(entities, e_link.row);
                                       found = true;
                                   }
                               }(Is),
                               0)
                         : 0),
                 ...);

                assert(found);
            });

            e_link.archetype_id = size_t(-1);
            e_link.row = size_t(-1);
        }

        template <typename F>
        void find_dead(F&& f, std::size_t limit = std::numeric_limits<std::size_t>::max(), std::size_t start_at = 0) {
            for (std::size_t i = start_at; i < std::min(start_at + limit, entities.size()); i++) {
                if (entities[i].archetype_id == size_t(-1) && entities[i].row == size_t(-1)) {
                    f(i);
                }
            }
        }

        template <typename... Subset> class System {
          private:
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

            static consteval decltype(auto) get_dirty() {
                constexpr auto dirty = get_dirty_impl2();

                return __::with_index_sequence(std::make_index_sequence<dirty.size()>{}, [&](auto... Is) {
                    return type_set<typeset::nth_t<dirty[Is], Subset...>...>{};
                });
            }

            using dirty_components = decltype(get_dirty());

            static constexpr auto archetypes = in_archetypes<std::remove_cvref_t<Subset>...>::value;
            std::array<void*, archetypes.size()> archetype_pointers;
            std::vector<runtime::Archetype*> runtime_archetypes{};

            std::array<std::vector<void**>, sizeof...(Subset)> data{};
            std::vector<std::size_t*> data_sizes{};

            template <std::size_t IX> void setup_archetype(const std::array<void*, sizeof...(Archetypes)>& arches) {
                constexpr auto arch_id = archetypes[IX];
                using archetype = index<arch_id>::T;
                archetype_pointers[IX] = arches[arch_id];
                archetype* x = (archetype*)arches[arch_id];
                auto subset = x->template get_subset<std::remove_cvref_t<Subset>...>();

                for (std::size_t i = 0; i < data.size(); i++) {
                    data[i].emplace_back(subset[i + 1]);
                }
                data_sizes.emplace_back((std::size_t*)subset[0]);
            }

            void setup_runtime_archetypes(
                std::vector<runtime::Archetype>& runtime_arches,
                std::unordered_map<std::type_index, std::vector<std::size_t>>& runtime_arches_table) {
                std::array<std::type_index, sizeof...(Subset)> needed = {typeid(Subset)...};

                std::vector<std::size_t> possible = runtime_arches_table[needed[0]];

                for (std::size_t i = 1; i < needed.size(); i++) {
                    auto opts = runtime_arches_table[needed[i]];

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
                }
            }

            System(const std::array<void*, sizeof...(Archetypes)>& arches,
                   std::vector<runtime::Archetype>& runtime_arches,
                   std::unordered_map<std::type_index, std::vector<std::size_t>>& runtime_arches_table) {
                data_sizes.reserve(archetypes.size());

                __::with_index_sequence(std::make_index_sequence<archetypes.size()>(), [&](auto... Is) {
                    ((data[Is].reserve(archetypes.size()), setup_archetype<Is>(arches)), ...);
                });

                setup_runtime_archetypes(runtime_arches, runtime_arches_table);
            }

          public:
            friend build;

            template <typename F> void run(F&& f, bool mark_dirty = false) {
                __::with_index_sequence(std::make_index_sequence<sizeof...(Subset)>{}, [&](auto... SubSeq) {
                    __::with_index_sequence(std::make_index_sequence<archetypes.size()>{}, [&](auto... ArchSeq) {
                        auto g = [&]<std::size_t ArchIx>() {
                            std::size_t size = *data_sizes[ArchIx];
                            for (std::size_t i = 0; i < size; i++) {
                                f((reinterpret_cast<typeset::nth_t<SubSeq, std::remove_reference_t<Subset>...>*>(
                                    *data[SubSeq][ArchIx])[i])...);
                            }
                        };

                        (g.template operator()<ArchSeq>(), ...);
                    });

                    for (std::size_t a = 0; a < runtime_archetypes.size(); a++) {
                        std::size_t size = *data_sizes[archetypes.size() + a];
                        for (std::size_t i = 0; i < size; i++) {
                            f((reinterpret_cast<typeset::nth_t<SubSeq, std::remove_reference_t<Subset>...>*>(
                                *data[SubSeq][archetypes.size() + a])[i])...);
                        }
                    }
                });

                if (mark_dirty) {
                    __::for_each_index(std::make_index_sequence<archetypes.size()>{}, [&](auto ArchI) {
                        constexpr auto ArchIx = ArchI.value;
                        using Arch = index<ArchIx>::T;

                        std::size_t range = *data_sizes[ArchIx];
                        auto* arch = reinterpret_cast<Arch*>(archetype_pointers[ArchIx]);

                        return false;
                    });
                }
            }
        };

        template <typename... Subset> System<Subset...> make_static_system() {
            return System<Subset...>(archetypes, {}, {});
        }

        template <typename... Subset> System<Subset...> make_system() {
            return System<Subset...>(archetypes, runtime_archetypes, components_of_runtime_archetype);
        }
    };
};
