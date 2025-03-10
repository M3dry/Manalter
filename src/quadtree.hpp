#pragma once

#include "print"
#include <cassert>
#include <cstdint>
#include <format>
#include <functional>
#include <raylib.h>
#include <utility>
#include <vector>

namespace quadtree {
    struct Box {
        Vector2 min;
        Vector2 max;

        bool collides(const Vector2& point) const {
            return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
        };
    };

    template <typename T> struct IDed {
        inline static uint64_t last_id = 0;
        uint64_t id;
        T val;

        IDed(T&& val) : val(std::forward<T>(val)), id(last_id + 1) {
            last_id++;
        }
    };

    template <typename T, typename Ix>
    bool lookup(const std::vector<IDed<T>>& vec, Ix& ix, const uint64_t& id) {
        if (vec.size() <= ix) return false;

        if (vec[ix].id == id) return true;

        for (Ix i = 0; i < vec.size(); i++) {
            if (vec[i].id == id) {
                ix = i;
                return true;
            }
        }

        return false;
    }

    using node_ix = uint16_t;
    static constexpr node_ix null = node_ix(-1);

    struct Node {
        bool subdivided = false;
        Box bbox;
        // [y][x]
        // [0, 0] - top left
        // [0, 1] - top right
        // [1, 0] - bottom left
        // [1, 1] - bottom right
        std::pair<node_ix, uint64_t> children[2][2]{
            {{null, -1}, {null, -1}},
            {{null, -1}, {null, -1}},
        };
        std::vector<std::pair<std::size_t, uint64_t>> data_ixs_ided;

        Node(Box bbox) : bbox(bbox) {};

        std::pair<node_ix, uint64_t>& operator[](int y, int x) {
            return children[y][x];
        }

        const std::pair<node_ix, uint64_t>& operator[](int y, int x) const {
            return children[y][x];
        }
    };

    template <uint8_t MaxPerNode, typename T> class QuadTree {
      public:
        // root node is always at 0 index
        std::vector<IDed<Node>> nodes;
        std::vector<IDed<std::pair<Vector2, T>>> data;

        QuadTree(Box bbox) : nodes({Node(bbox)}) {};

        bool insert(Vector2 pos, T&& t) {
            return insert({0, -1}, pos, std::forward<T>(t)).has_value();
        }

        void print(std::function<void(const T&, const char*)> print_t) const {
            std::println("NODES:");
            for (const auto& node : nodes) {
                std::println("ID: {}; VAL:", node.id);
                std::println("\tbbox: {{ .min = [{}, {}], .max = [{}, {}] }}", node.val.bbox.min.x, node.val.bbox.min.y, node.val.bbox.max.x, node.val.bbox.max.y);
                std::println("\tsubdivided: {}", node.val.subdivided);
                std::print("\tchildren: [");
                for (int y = 0; y < 2; y++) {
                    for (int x = 0; x < 2; x++) {
                        std::print(" {{ {}, {} }}", node.val[y, x].first, node.val[y, x].second);
                    }
                }
                std::println(" ]");
                std::print("\tdata_ixs_ided: [");
                for (const auto& [ix, id] : node.val.data_ixs_ided) {
                    std::print(" {{ {}, {} }}", ix, id);
                }
                std::println(" ]");
            }

            std::println("\nDATA: ");
            for (const auto& dat : data) {
                std::println("ID: {}; VAL:", dat.id);
                std::println("\tposition: [{}, {}]", dat.val.first.x, dat.val.first.y);
                std::println("\tUSER DATA:");
                print_t(dat.val.second, "\t\t");
            }
        }

      private:
        std::optional<node_ix> insert(std::pair<node_ix, uint64_t> ix_ided, Vector2 pos, T&& t) {
            node_ix parent_ix = ix_ided.first;
            if (parent_ix == 0) {
            } else if (!lookup(nodes, parent_ix, ix_ided.second)) {
                return std::nullopt;
            }
            auto& parent = nodes[parent_ix].val;

            if (!parent.bbox.collides(pos)) return std::nullopt;

            if (parent.data_ixs_ided.size() < MaxPerNode) {
                data.emplace_back(std::pair{ pos, std::forward<T>(t) });
                parent.data_ixs_ided.emplace_back(data.size() - 1, data[data.size() - 1].id);

                return parent_ix;
            }

            if (!parent.subdivided) {
                subdivide(parent_ix);
            }
            // AFTER this point `parent` might be invalidated, use `parent_ix`

            for (int i = 0; i < 2; i++) {
                for (int j = 0; i < 2; j++) {
                    auto child_ix = nodes[parent_ix].val.children[i][j];
                    if (auto new_ix = insert(child_ix, pos, std::forward<T>(t)); new_ix) {
                        nodes[parent_ix].val[i, j].first = *new_ix;
                        return parent_ix;
                    }
                }
            }

            return std::nullopt;
        }

        void subdivide(node_ix ix) {
            auto& parent = nodes[ix].val;
            if (parent.subdivided) return;

            float mid_x = (parent.bbox.min.x + parent.bbox.max.x) / 2.0f;
            float mid_y = (parent.bbox.min.y + parent.bbox.max.y) / 2.0f;

#define PARENT nodes[ix].val
            PARENT[1, 0] = add_node(Box{.min = PARENT.bbox.min, .max = {mid_x, mid_y}});
            PARENT[1, 1] = add_node(Box{
                .min = {mid_x, PARENT.bbox.min.y},
                .max = {PARENT.bbox.max.x, mid_y},
            });
            PARENT[0, 0] = add_node(Box{
                .min = { PARENT.bbox.min.x, mid_y },
                .max = { mid_x, PARENT.bbox.max.y },
            });
            PARENT[0, 1] = add_node(Box{
                .min = { mid_x, mid_y },
                .max = PARENT.bbox.max,
            });

            for (const auto& [_data_ix, id] : PARENT.data_ixs_ided) {
                auto data_ix = _data_ix;
                if (!lookup(data, data_ix, id)) {
                    assert(false && "invalidated ix");
                }

                const auto& pos = data[data_ix].val.first;

                if (pos.y < mid_y) {
                    if (pos.x < mid_x) {
                        nodes[PARENT[1, 0].first].val.data_ixs_ided.emplace_back(data_ix, id);
                    } else {
                        nodes[PARENT[1, 1].first].val.data_ixs_ided.emplace_back(data_ix, id);
                    }
                } else {
                    if (pos.x < mid_x) {
                        nodes[PARENT[0, 0].first].val.data_ixs_ided.emplace_back(data_ix, id);
                    } else {
                        nodes[PARENT[0, 1].first].val.data_ixs_ided.emplace_back(data_ix, id);
                    }
                }
            }

            PARENT.data_ixs_ided.clear();
            PARENT.data_ixs_ided.shrink_to_fit();
            PARENT.subdivided = true;
#undef PARENT
        }

        inline std::pair<node_ix, uint64_t> add_node(Box&& bbox) {
            nodes.emplace_back(bbox);
            return { nodes.size() - 1, nodes[nodes.size() - 1].id };
        }
    };
}
