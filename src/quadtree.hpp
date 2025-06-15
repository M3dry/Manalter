#pragma once

#include "hitbox.hpp"
#include "print"
#include <cassert>
#include <concepts>
#include <cstdint>
#include <format>
#include <functional>
#include <raylib.h>
#include <raymath.h>
#include <type_traits>
#include <utility>
#include <vector>

namespace quadtree {
    template <typename T>
    concept HasPosition = requires(T position, const Vector2& vec) {
        { position.position() } -> std::convertible_to<Vector2>;
        { position.set_position(vec) } -> std::same_as<void>;
    };

    struct Box {
        Vector2 min;
        Vector2 max;

        Box(const Vector2& min, const Vector2& max) : min(min), max(max) {};
        Box(const Rectangle& rec) : min(rec.x, rec.y), max(rec.x + rec.width, rec.y + rec.height) {
            if (min.x > max.x) {
                std::swap(min, max);
            }
        };

        operator Rectangle() const {
            return Rectangle{.x = min.x, .y = min.y, .width = max.x - min.x, .height = max.y - min.y};
        }

        bool contains(const Vector2& point) const {
            return point.x >= min.x && point.x <= max.x && point.y >= min.y && point.y <= max.y;
        };

        // touching isn't counted as intersection
        bool intersect(const Box& other) const {
            return (min.x < other.max.x && max.x > other.min.x) && (min.y < other.max.y && max.y > other.min.y);
        }

        bool wrap_around(Vector2& v) const {
            if (contains(v)) return false;

            float width = max.x - min.x;
            float height = max.y - min.y;

            v.x = min.x + std::fmod(std::fmod((v.x - min.x), width) + width, width);
            v.y = min.y + std::fmod(std::fmod((v.y - min.y), height) + height, height);

            return true;
        }

        void draw(Color col, bool two_d) const {
            shapes::Polygon bbox = static_cast<Rectangle>(*this);

            if (two_d) {
                bbox.draw_lines_2D(col);
            } else {
                bbox.draw_lines_3D(col, 1.0f);
            }
        }
    };

    template <typename T> struct IDed {
        inline static uint64_t last_id = 0;
        uint64_t id;
        T val;

        template <typename U>
            requires(!std::same_as<std::decay_t<U>, IDed>)
        IDed(U&& val) : id(++last_id), val(std::forward<T>(val)) {
        }

        template <typename... Args>
            requires std::constructible_from<T, Args...>
        IDed(Args&&... args) : id(++last_id), val(std::forward<Args>(args)...) {
        }

        IDed(IDed&& i) noexcept : id(i.id), val(std::move(i.val)) {
            i.id = static_cast<uint64_t>(-1);
        }
        IDed& operator=(IDed&&) noexcept = default;
    };

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

        void set_position(const Vector2& p) {
            _pos = p;
        }
    };

    static_assert(HasPosition<pos<int>>);

    template <typename T> struct IDVec {
        std::vector<IDed<T>> vec;

        template <typename Ix> bool lookup(Ix& ix, const uint64_t& id) const {
            if (ix == static_cast<Ix>(-1)) return false;

            if (vec.size() > ix && (id == static_cast<uint64_t>(-1) || vec[ix].id == id)) return true;

            if (id == static_cast<uint64_t>(-1)) return false;

            for (std::size_t i = 0; i < vec.size(); i++) {
                if (vec[i].id == id) {
                    ix = i;
                    return true;
                }
            }

            return false;
        }

        std::vector<IDed<T>>& operator*() {
            return vec;
        }

        const std::vector<IDed<T>>& operator*() const {
            return vec;
        }

        std::vector<IDed<T>>* operator->() {
            return &vec;
        }

        std::vector<IDed<T>> const* const operator->() const {
            return &vec;
        }

        T& operator[](std::size_t& ix, uint64_t id) {
            if (!lookup(ix, id)) {
                assert(false);
            }

            return vec[ix].val;
        }

        T& operator[](std::size_t ix, uint64_t id) {
            auto _ix = ix;
            if (!lookup(_ix, id)) {
                assert(false);
            }

            return vec[ix].val;
        }

        const T& operator[](std::size_t& ix, uint64_t id) const {
            if (!lookup(ix, id)) {
                assert(false);
            }

            return vec[ix].val;
        }

        const T& operator[](std::size_t ix, uint64_t id) const {
            auto _ix = ix;
            if (!lookup(_ix, id)) {
                assert(false);
            }

            return vec[_ix].val;
        }
    };

    template <uint8_t MaxPerNode, typename T, bool __SkipPositionCheck = false>
        requires __SkipPositionCheck || HasPosition<T>
    class QuadTree {
      public:
        // root node is always at 0 index
        IDVec<Node> nodes;
        IDVec<T> data;

        QuadTree(Box bbox) : nodes() {
            nodes->emplace_back(Node(bbox));
        };

        operator QuadTree<MaxPerNode, T, false>&() {
            return reinterpret_cast<QuadTree<MaxPerNode, T, false>&>(*this);
        }

        operator QuadTree<MaxPerNode, T, true>&() {
            return reinterpret_cast<QuadTree<MaxPerNode, T, true>&>(*this);
        }

        template <typename... Args> std::pair<std::size_t, uint64_t> insert(Args&&... args) {
            data->emplace_back(std::forward<Args>(args)...);

            if (!_insert(0, -1, data->size() - 1, data->back().id)) {
                assert(false && "How???");
            }

            return {data->size() - 1, data->back().id};
        }

        void remove(std::size_t data_ix) {
            if (data_ix >= data->size()) return;

            std::swap(data.vec[data_ix], data.vec.back());
            data->pop_back();
        }

        void remove(std::size_t data_ix, uint64_t data_id) {
            if (data.lookup(data_ix, data_id)) remove(data_ix);
        }

        // assumes the function doesn't change position
        void search_by(std::function<bool(const Box&)> check_box, std::function<bool(const T&)> check_data,
                       std::function<void(T&, std::size_t)> f) {
            search_by(0, -1, check_box, check_data, f);
        }

        void in_box(const Box& bbox, std::function<void(const T&, std::size_t)> f) {
            search_by([&bbox](const Box& other) { return bbox.intersect(other); },
                      [&bbox](const T& t) { return bbox.contains(t.position()); }, f);
        }

        std::optional<node_ix> closest_to(const Vector2& point) const {
            std::size_t closest = static_cast<std::size_t>(-1);
            float closest_dist = -1.0f;

            for (std::size_t i = 0; i < data->size(); i++) {
                auto dist = Vector2DistanceSqr(point, data.vec[i].val.position());
                if (closest == static_cast<std::size_t>(-1) || closest_dist > dist) {
                    closest = i;
                    closest_dist = dist;
                }
            }

            return closest;
        }

        void rebuild() {
            auto root_bbox = nodes[0, -1].bbox;
            nodes->clear();
            nodes->emplace_back(Node(root_bbox));

            for (std::size_t ix = 0; ix < data->size(); ix++) {
                _insert(0, -1, ix, data.vec[ix].id);
            }
        }

        void reinsert(std::size_t data_ix) {
            data.vec[data_ix].id = ++data.vec[data_ix].last_id;
            _insert(0, -1, data_ix, data.vec[data_ix].id);
        }

        void prune() {
            prune(0, -1);
        }

        void print(std::function<void(const T&, const char*)> print_t) const {
            std::println("NODES:");
            for (const auto& node : nodes.vec) {
                std::println("ID: {}; VAL:", node.id);
                std::println("\tbbox: {{ .min = [{}, {}], .max = [{}, {}] }}", node.val.bbox.min.x, node.val.bbox.min.y,
                             node.val.bbox.max.x, node.val.bbox.max.y);
                std::println("\tsubdivided: {}", node.val.subdivided);
                std::print("\tchildren: [");
                for (int y = 0; y < 2; y++) {
                    for (int x = 0; x < 2; x++) {
                        std::print(" {{ [{}, {}]: ix: {}, id :{} }}", y, x, node.val[y, x].first,
                                   node.val[y, x].second);
                    }
                }
                std::println(" ]");
                std::print("\tdata_ixs_ided: [");
                for (const auto& [ix, id] : node.val.data_ixs_ided) {
                    std::print(" {{ ix: {}, id: {} }}", ix, id);
                }
                std::println(" ]");
            }

            std::println("\nDATA: ");
            for (const auto& dat : *data) {
                std::println("ID: {}; VAL:", dat.id);
                std::println("\tposition: [{}, {}]", dat.val.position().x, dat.val.position().y);
                std::println("\tUSER DATA:");
                print_t(dat.val, "\t\t");
            }
        }

        void draw_bbs(Color col, bool two_d) {
            draw_bbs(col, 0, -1, two_d);
        }

      private:
        std::optional<node_ix> _insert(node_ix parent_ix, uint64_t parent_id, std::size_t dat_ix, uint64_t dat_id) {
            const auto& parent = nodes[parent_ix, parent_id];

            if (!parent.bbox.contains(data.vec[dat_ix].val.position())) return std::nullopt;

            if (!parent.subdivided && parent.data_ixs_ided.size() + 1 <= MaxPerNode) {
                nodes[parent_ix, parent_id].data_ixs_ided.emplace_back(dat_ix, dat_id);

                return parent_ix;
            }

            if (!parent.subdivided) {
                subdivide(parent_ix, parent_id);
            }
            // AFTER this point `parent` might be invalidated, use `parent_ix`
            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 2; j++) {
                    auto& [child_ix, child_id] = nodes[parent_ix, parent_id][i, j];

                    if (auto new_ix = _insert(child_ix, child_id, dat_ix, dat_id); new_ix) {
                        nodes[parent_ix, parent_id][i, j].first = *new_ix;

                        return parent_ix;
                    }
                }
            }

            return std::nullopt;
        }

        void subdivide(node_ix ix, uint64_t ix_id) {
            auto& parent = nodes[ix, ix_id];
            if (parent.subdivided) return;

            float mid_x = (parent.bbox.min.x + parent.bbox.max.x) / 2.0f;
            float mid_y = (parent.bbox.min.y + parent.bbox.max.y) / 2.0f;

#define PARENT nodes[ix, ix_id]
            PARENT[0, 0] = add_node(Box(PARENT.bbox.min, {mid_x, mid_y}));
            PARENT[0, 1] = add_node(Box({mid_x, PARENT.bbox.min.y}, {PARENT.bbox.max.x, mid_y}));
            PARENT[1, 0] = add_node(Box({PARENT.bbox.min.x, mid_y}, {mid_x, PARENT.bbox.max.y}));
            PARENT[1, 1] = add_node(Box({mid_x, mid_y}, PARENT.bbox.max));

            for (const auto& [_data_ix, id] : PARENT.data_ixs_ided) {
                auto data_ix = _data_ix;
                if (!data.lookup(data_ix, id)) {
                    continue;
                }

                const auto& pos = data.vec[data_ix].val.position();

                if (pos.y < mid_y) {
                    if (pos.x < mid_x) {
                        nodes[PARENT[0, 0].first, PARENT[0, 0].second].data_ixs_ided.emplace_back(data_ix, id);
                    } else {
                        nodes[PARENT[0, 1].first, PARENT[0, 0].second].data_ixs_ided.emplace_back(data_ix, id);
                    }
                } else {
                    if (pos.x < mid_x) {
                        nodes[PARENT[1, 0].first, PARENT[0, 0].second].data_ixs_ided.emplace_back(data_ix, id);
                    } else {
                        nodes[PARENT[1, 1].first, PARENT[0, 0].second].data_ixs_ided.emplace_back(data_ix, id);
                    }
                }
            }

            PARENT.data_ixs_ided.clear();
            PARENT.data_ixs_ided.shrink_to_fit();
            PARENT.subdivided = true;
#undef PARENT
        }

        std::pair<node_ix, uint64_t> add_node(Box&& bbox) {
            nodes->emplace_back(bbox);

            return {nodes->size() - 1, nodes->back().id};
        }

        // assumes `ix` is valid
        void search_by(node_ix ix, uint64_t ix_id, std::function<bool(const Box&)> check_box,
                       std::function<bool(const T&)> check_point, std::function<void(T&, std::size_t)> f) {
            auto& node = nodes[ix, ix_id];

            if (!check_box(node.bbox)) return;

            if (!node.subdivided) {
                for (auto& [data_ix, data_id] : node.data_ixs_ided) {
                    if (!data.lookup(data_ix, data_id)) continue;

                    if (check_point(data.vec[data_ix].val)) {
                        f(data.vec[data_ix].val, data_ix);
                    }
                }

                return;
            }

            for (int x = 0; x < 2; x++) {
                for (int y = 0; y < 2; y++) {
                    auto& [child_ix, child_id] = node[x, y];

                    search_by(child_ix, child_id, check_box, check_point, f);
                }
            }

        }

        std::size_t prune(node_ix ix, uint64_t ix_id) {
            {
                std::size_t i = 0;
                while (i < nodes[ix, ix_id].data_ixs_ided.size()) {
                    auto& [data_ix, data_id] = nodes[ix, ix_id].data_ixs_ided[i];
                    if (!data.lookup(data_ix, data_id)) {

                        std::swap(nodes[ix, ix_id].data_ixs_ided[i], nodes[ix, ix_id].data_ixs_ided.back());
                        nodes[ix, ix_id].data_ixs_ided.pop_back();
                        continue;
                    }

                    i++;
                }
            }

            auto size = nodes[ix, ix_id].data_ixs_ided.size();
            if (!nodes[ix, ix_id].subdivided) return size;

            bool all_leaf = true;
            std::size_t children_size = 0;
            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 2; j++) {
                    auto& [child_ix, child_id] = nodes[ix, ix_id][i, j];

                    children_size += prune(child_ix, child_id);
                    if (nodes[child_ix, child_id].subdivided) all_leaf = false;
                }
            }

            auto total_size = size + children_size;
            if (!(all_leaf && total_size <= MaxPerNode)) {
                return total_size;
            }

            nodes[ix, ix_id].data_ixs_ided.reserve(total_size);
            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 2; j++) {
                    auto& [child_ix, child_id] = nodes[ix, ix_id][i, j];
                    auto& child = nodes[child_ix, child_id];
                    nodes[ix, ix_id].data_ixs_ided.insert(nodes[ix, ix_id].data_ixs_ided.end(),
                                                          child.data_ixs_ided.begin(), child.data_ixs_ided.end());
                    bool moved = nodes->back().id == ix_id;

                    if (!nodes.lookup(child_ix, child_id)) {
                        assert(false);
                    }
                    std::swap(nodes.vec[child_ix], nodes.vec.back());
                    nodes->pop_back();

                    if (moved) {
                        ix = child_ix;
                    }
                    nodes[ix, ix_id][i, j] = {null, -1};
                }
            }

            nodes[ix, ix_id].subdivided = false;
            return total_size;
        }

        void draw_bbs(Color col, node_ix ix, uint64_t ix_id, bool two_d) {
            auto& node = nodes[ix, ix_id];

            node.bbox.draw(col, two_d);

            if (!node.subdivided) return;

            for (int i = 0; i < 2; i++) {
                for (int j = 0; j < 2; j++) {
                    auto& [child_ix, child_id] = node[i, j];

                    draw_bbs(col, child_ix, child_id, two_d);
                }
            }
        }
    };
}
