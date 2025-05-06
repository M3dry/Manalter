#pragma once

#include <cassert>
#include <compare>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <new>
#include <optional>

#include "seria_deser.hpp"

template <typename T> class RingBuffer {
  public:
    template <typename RB> class Iter {
      public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::random_access_iterator_tag;

        Iter() {
        }
        Iter(RB* rb, std::size_t normalized_ix = 0)
            : normalized_ix(static_cast<difference_type>(normalized_ix)), rb(rb) {
        }

        bool operator==(const Iter& sentinel) const {
            return normalized_ix == sentinel.normalized_ix && rb == sentinel.rb;
        }

        bool operator!=(const Iter& sentinel) const {
            return !operator==(sentinel);
        }

        decltype(auto) operator*() const {
            return (*rb)[static_cast<std::size_t>(normalized_ix)];
        }

        Iter& operator++() {
            normalized_ix++;
            return *this;
        }

        Iter operator++(int) {
            Iter tmp = *this;
            ++*this;
            return tmp;
        }

        Iter& operator--() {
            normalized_ix--;
            return *this;
        }

        Iter operator--(int) {
            Iter tmp = *this;
            --*this;
            return tmp;
        }

        Iter& operator+=(const difference_type& by) {
            normalized_ix += by;
            return *this;
        }

        Iter& operator-=(const difference_type& by) {
            normalized_ix -= by;
            return *this;
        }

        friend Iter operator+(const Iter& iter, const difference_type& by) {
            Iter tmp = iter;
            tmp += by;
            return tmp;
        }

        friend Iter operator+(const difference_type& by, const Iter& iter) {
            return iter + by;
        }

        friend Iter operator-(const Iter& iter, const difference_type& by) {
            Iter tmp = iter;
            tmp -= by;
            return tmp;
        }

        friend difference_type operator-(const Iter& iter1, const Iter& iter2) {
            return iter1.normalized_ix - iter2.normalized_ix;
        }

        decltype(auto) operator[](const difference_type& i) const {
            return (*rb)[static_cast<std::size_t>(normalized_ix + i)];
        }

        friend std::strong_ordering operator<=>(const Iter& lhs, const Iter& rhs) {
            return lhs.normalized_ix <=> rhs.normalized_ix;
        }

      private:
        difference_type normalized_ix = 0;
        RB* rb;
    };

    using iterator = Iter<RingBuffer<T>>;
    using const_iterator = Iter<const RingBuffer<T>>;

    RingBuffer() {
    }

    RingBuffer(std::size_t capacity) : capacity(capacity), raw_buffer() {
        raw_buffer.reset(static_cast<std::byte*>(::operator new(capacity * sizeof(T), std::align_val_t{alignof(T)})));
    }

    RingBuffer(RingBuffer&& rb)
        : start_ix(rb.start_ix), end_ix(rb.end_ix), capacity(rb.capacity), raw_buffer(std::move(rb.raw_buffer)) {
    }
    RingBuffer& operator=(RingBuffer&& rb) {
        if (this != &rb) {
            start_ix = rb.start_ix;
            end_ix = rb.end_ix;
            capacity = rb.capacity;
            raw_buffer = std::move(rb.raw_buffer);

            rb.start_ix = 0;
            rb.end_ix = std::nullopt;
            rb.capacity = 0;
        }

        return *this;
    }

    ~RingBuffer() {
        if (!raw_buffer) return;

        clear();
        raw_buffer.reset();
    }

    operator RingBuffer<const T>&() const {
        return *reinterpret_cast<const RingBuffer<const T>*>(this);
    }

    operator const RingBuffer<const T>&() const {
        return *reinterpret_cast<const RingBuffer<const T>*>(this);
    }

    inline bool empty() const {
        return !end_ix;
    }

    void clear() {
        if (empty()) return;
        for (std::size_t i = start_ix; i != *end_ix; i = (i + 1) % capacity) {
            std::destroy_at<T>(&buffer()[i]);
        }

        start_ix = 0;
        end_ix = std::nullopt;
    }

    std::size_t size() const {
        if (empty()) {
            assert(start_ix == 0);
            return 0;
        } else if (start_ix <= *end_ix) {
            return *end_ix - start_ix + 1;
        } else {
            return capacity - start_ix + *end_ix + 1;
        }
    }

    T& operator[](std::size_t normalized_ix) const {
        return buffer()[from_normalized(normalized_ix)];
    }

    T& front() const {
        return buffer()[start_ix];
    }

    T& back() const {
        return buffer()[*end_ix];
    }

    void pop_front() {
        if (empty()) return;

        std::destroy_at<T>(&buffer()[start_ix]);
        if (start_ix == *end_ix) {
            start_ix = 0;
            end_ix = std::nullopt;
            return;
        }

        start_ix++;
        if (start_ix >= capacity) start_ix = 0;
    }

    void pop_back() {
        if (empty()) return;

        std::destroy_at<T>(&buffer()[*end_ix]);

        if (start_ix == *end_ix) {
            start_ix = 0;
            end_ix = std::nullopt;
            return;
        }

        if (*end_ix == 0) {
            end_ix = capacity - 1;
        } else {
            (*end_ix)--;
        }
    }

    template <typename... Args> void emplace_front(Args&&... args) {
        if (size() == capacity) resize(capacity * 2 + 1);

        if (empty()) {
            assert(start_ix == 0);
            end_ix = 0;
        } else if (start_ix == 0) {
            start_ix = capacity - 1;
        } else {
            start_ix--;
        }

        std::construct_at(&buffer()[start_ix], std::forward<Args>(args)...);
    }

    template <typename U>
        requires std::is_same_v<std::remove_cvref_t<U>, T>
    void push_front(U&& value) {
        emplace_front(std::forward<U>(value));
    }

    template <typename... Args> void emplace_back(Args&&... args) {
        if (size() == capacity) resize(capacity * 2 + 1);

        if (empty()) {
            assert(start_ix == 0);
            end_ix = 0;
        } else {
            end_ix = (*end_ix + 1) % capacity;
        }

        std::construct_at(&buffer()[*end_ix], std::forward<Args>(args)...);
    }

    template <typename U>
        requires std::is_same_v<std::remove_cvref_t<U>, T>
    void push_back(U&& value) {
        emplace_back(std::forward<U>(value));
    }

    // clang-format off
    iterator begin() { return Iter(this); }
    const_iterator begin() const { return Iter(this); }
    const_iterator cbegin() const { return begin(); }
    std::reverse_iterator<iterator> rbegin() { return std::reverse_iterator(end()); }
    std::reverse_iterator<const_iterator> rbegin() const { return std::reverse_iterator(end()); }
    std::reverse_iterator<const_iterator> crbegin() const { return std::reverse_iterator(cend()); }

    iterator end() { return Iter(this, size()); }
    const_iterator end() const { return Iter(this, size()); }
    const_iterator cend() const { return end(); }
    std::reverse_iterator<iterator> rend() { return std::reverse_iterator(begin()); }
    std::reverse_iterator<const_iterator> rend() const { return std::reverse_iterator(begin()); }
    std::reverse_iterator<const_iterator> crend() const { return std::reverse_iterator(cbegin()); }
    // clang-format on

    void serialize(std::ostream& out) const {
        seria_deser::serialize(size(), out);
        for (const auto& x : *this) {
            seria_deser::serialize(x, out);
        }
    }

    static RingBuffer deserialize(std::istream& in, version v) {
        std::size_t size = seria_deser::deserialize<std::size_t>(in, v);

        RingBuffer rb(size);
        for (std::size_t i = 0; i < size; i++) {
            rb.emplace_back(seria_deser::deserialize<T>(in, v));
        }

        return rb;
    }

  private:
    struct Deleter {
        static void operator()(std::byte* ptr) {
            ::operator delete(ptr, std::align_val_t{alignof(T)});
        }
    };

    std::size_t start_ix = 0;
    std::optional<std::size_t> end_ix = std::nullopt;
    std::size_t capacity = 0;
    std::unique_ptr<std::byte[], Deleter> raw_buffer;

    template <typename U = T>
    inline typename std::enable_if<!std::is_const_v<U>, T*>::type buffer() {
        return reinterpret_cast<T*>(raw_buffer.get());
    }

    template <typename U = T>
    inline typename std::enable_if<!std::is_const_v<U>, T* const>::type buffer() const {
        return reinterpret_cast<T*>(raw_buffer.get());
    }

    template <typename U = T>
    inline typename std::enable_if<std::is_const_v<U>, T const* const>::type buffer() const {
        return reinterpret_cast<T*>(raw_buffer.get());
    }

    std::size_t from_normalized(std::size_t ix) const {
        if (empty()) return static_cast<std::size_t>(-1);

        return (start_ix + ix) % capacity;
    }

    void resize(std::size_t new_capacity) {
        assert(new_capacity > capacity);

        auto count = size();

        auto new_buffer = static_cast<T*>(::operator new(new_capacity * sizeof(T), std::align_val_t{alignof(T)}));
        if (end_ix) {
            for (std::size_t i = 0; i < count; i++) {
                auto src_i = from_normalized(i);

                if constexpr (std::is_move_constructible_v<T>) {
                    std::construct_at(&new_buffer[i], std::move(buffer()[src_i]));
                } else {
                    static_assert(std::is_copy_constructible_v<T>,
                                  "T must be copy-constructible or move-constructible");
                    std::construct_at(&new_buffer[i], buffer()[src_i]);
                }

                std::destroy_at(&buffer()[src_i]);
            }
        }

        start_ix = 0;
        end_ix = count == 0 ? std::nullopt : std::make_optional(count - 1);
        capacity = new_capacity;
        raw_buffer.reset(reinterpret_cast<std::byte*>(new_buffer));
    }
};
