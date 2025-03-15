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

        Box(const Vector2& min, const Vector2& max) : min(min), max(max) {};
        Box(const Rectangle& rec) : min(rec.x, rec.y), max(rec.x + rec.width, rec.y + rec.height) {
            if (min.x > max.x) {
                std::swap(min, max);
            }
        };

        bool collides(const Vector2& point) const {
            return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
        };

        // touching isn't counted as intersection
        bool intersect(const Box& other) const {
            return (min.x < other.max.x && max.x > other.min.x) && (min.y < other.max.y && max.y > other.min.y);
        }
    };

    template <typename T> struct IDed {
        inline static uint64_t last_id = 0;
        uint64_t id;
        T val;

        IDed(T&& val) : val(std::forward<T>(val)), id(last_id + 1) {
            last_id++;
        }
    };

    template <typename T, typename Ix> bool lookup(const std::vector<IDed<T>>& vec, Ix& ix, const uint64_t& id) {
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

    template <typename T>
    concept HasPosition = requires(T position) {
        { position.position() } -> std::convertible_to<Vector2>;
    };

    template <typename T> struct pos {
        Vector2 _pos;
        T t;

        pos(Vector2 pos, T&& t) : _pos(pos), t(std::forward<T>(t)) {};

        T& operator*() {
            return t;
        }
        const T& operator*() const {
            return t;
        }

        Vector2& position() {
            return _pos;
        }

        const Vector2& position() const {
            return _pos;
        }
    };

    static_assert(HasPosition<pos<int>>);

    template <uint8_t MaxPerNode, HasPosition T> class QuadTree {
      public:
        // root node is always at 0 index
        std::vector<IDed<Node>> nodes;
        std::vector<IDed<T>> data;

        QuadTree(Box bbox) : nodes({Node(bbox)}) {};

        std::pair<std::size_t, uint64_t> insert(T&& t) {
            if (!insert(0, -1, std::forward<T>(t))) {
                assert(false && "How???");
            }

            return {data.size() - 1, data[data.size() - 1].id};
        }

        void remove(std::size_t data_ix) {
            auto size = data.size();

            if (data_ix >= size) return;

            if (data_ix != size - 1) {
                std::swap(data[data_ix], data[size - 1]);
            }

            data.pop_back();
        }

        void remove(std::size_t data_ix, uint64_t data_id) {
            if (lookup(data, data_ix, data_id)) remove(data_ix);
        }

        void search_by(std::function<bool(const Box&)> check_box, std::function<bool(const Vector2&, std::size_t)> check_point, std::function<void(std::size_t)> f) {
            search_by(0, check_box, check_point, f);
        }

        void in_box(const Box& bbox, std::function<void(std::size_t)> f) {
            search_by([&bbox](const Box& other) { return bbox.intersect(other); },
                      [&bbox](const Vector2& p, auto _) { return bbox.collides(p); }, f);
        }

        void delete_by(std::function<bool(T&)> f) {
            delete_by(0, f);
        }

        // NOTE: don't think I'll need pruning, but this should be a good starting point
        /*void prune() {*/
        /*    prune(0, -1);*/
        /*}*/

        void print(std::function<void(const T&, const char*)> print_t) const {
            std::println("NODES:");
            for (const auto& node : nodes) {
                std::println("ID: {}; VAL:", node.id);
                std::println("\tbbox: {{ .min = [{}, {}], .max = [{}, {}] }}", node.val.bbox.min.x, node.val.bbox.min.y,
                             node.val.bbox.max.x, node.val.bbox.max.y);
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
                std::println("\tposition: [{}, {}]", dat.val.position().x, dat.val.position().y);
                std::println("\tUSER DATA:");
                print_t(dat.val, "\t\t");
            }
        }

      private:
        std::optional<node_ix> insert(node_ix ix, uint64_t ix_id, T&& t) {
            node_ix parent_ix = ix;
            if (parent_ix == 0) {
            } else if (!lookup(nodes, parent_ix, ix_id)) {
                return std::nullopt;
            }
            auto& parent = nodes[parent_ix].val;

            if (!parent.bbox.collides(t.position())) return std::nullopt;

            if (parent.data_ixs_ided.size() < MaxPerNode) {
                data.emplace_back(std::forward<T>(t));
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
                    if (auto new_ix = insert(child_ix.first, child_ix.second, std::forward<T>(t)); new_ix) {
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
            PARENT[1, 0] = add_node(Box(PARENT.bbox.min, {mid_x, mid_y}));
            PARENT[1, 1] = add_node(Box({mid_x, PARENT.bbox.min.y}, {PARENT.bbox.max.x, mid_y}));
            PARENT[0, 0] = add_node(Box({PARENT.bbox.min.x, mid_y}, {mid_x, PARENT.bbox.max.y}));
            PARENT[0, 1] = add_node(Box({mid_x, mid_y}, PARENT.bbox.max));

            for (const auto& [_data_ix, id] : PARENT.data_ixs_ided) {
                auto data_ix = _data_ix;
                if (!lookup(data, data_ix, id)) {
                    continue;
                }

                const auto& pos = data[data_ix].val.position();

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
            return {nodes.size() - 1, nodes[nodes.size() - 1].id};
        }

        /*std::size_t prune(node_ix ix, uint64_t ix_id) {*/
        /*    if (!lookup(nodes, ix, ix_id)) {*/
        /*        return 0;*/
        /*    }*/
        /**/
        /*    auto& node = nodes[ix].val;*/
        /*    if (!node.subdivided) {*/
        /*        return node.data_ixs_ided.size();*/
        /*    }*/
        /**/
        /*    int data_sum = 0;*/
        /*    for (int x = 0; x < 2; x++) {*/
        /*        for (int y = 0; y < 2; y++) {*/
        /*            if (!lookup(nodes, node[x, y].first, node[x, y].second)) {*/
        /*                assert(false && "shouldn't be possible to have an uninitialzed quadrant and `subdivided` be
         * true");*/
        /*            }*/
        /**/
        /*            data_sum += nodes[node[x, y].first].val.data_ixs_ided.size();*/
        /**/
        /*            if (data_sum >= MaxPerNode) {*/
        /*                break;*/
        /*            }*/
        /*        }*/
        /*    }*/
        /**/
        /**/
        /*}*/

        // assumes `ix` is valid
        void search_by(node_ix ix, std::function<bool(const Box&)> check_box,
                       std::function<bool(const Vector2&, std::size_t)> check_point, std::function<void(std::size_t)> f) {
            auto& node = nodes[ix].val;

            if (!check_box(node.bbox)) return;

            if (!node.subdivided) {
                for (auto& [data_ix, data_id] : node.data_ixs_ided) {
                    if (!lookup(data, data_ix, data_id)) continue;

                    if (check_point(data[data_ix].val.position(), data_ix)) {
                        f(data_ix);
                    }
                }
                return;
            }

            for (int x = 0; x < 2; x++) {
                for (int y = 0; y < 2; y++) {
                    auto& [child_ix, child_id] = node[x, y];
                    if (!lookup(nodes, child_ix, child_id)) {
                        assert(false && "A subdivided node should always have values");
                    }

                    search_by(child_ix, check_box, check_point, f);
                }
            }
        }

        void delete_by(node_ix ix, std::function<bool(T&)> f) {
            auto& node = nodes[ix].val;

            if (!node.subdivided) {
                int i = 0;
                auto size = node.data_ixs_ided.size();
                while (i < size) {
                    auto& [data_ix, data_id] = node.data_ixs_ided[i];
                    // remove the already removed element from data_ixs
                    if (!lookup(data, data_ix, data_id)) {
                        std::swap(node.data_ixs_ided[i], node.data_ixs_ided[size - 1]);
                        size--;
                        node.data_ixs_ided.pop_back();

                        continue;
                    }

                    if (f(data[data_ix].val)) {
                        std::swap(data[data_ix], data[data.size() - 1]);
                        data.pop_back();

                        std::swap(node.data_ixs_ided[i], node.data_ixs_ided[size - 1]);
                        size--;
                        node.data_ixs_ided.pop_back();

                        continue;
                    }

                    i++;
                }
            }
        }
    };
}
