#pragma once

#include <cassert>
#include <cstddef>
#include <optional>
#include <print>
#include <vector>

namespace ringbuffer {
    template <typename T> class RingBuffer {
      public:
        RingBuffer(std::size_t length) : buffer(length), buffer_size(length) {
        }
        RingBuffer(RingBuffer&) = default;
        RingBuffer(RingBuffer&& rb) noexcept
            : buffer{std::move(rb.buffer)}, start(rb.start), end(rb.end), empty(rb.empty) {
        }
        RingBuffer& operator=(const RingBuffer&) = default;
        RingBuffer& operator=(RingBuffer&&) = default;

        // pushes element infront of the head
        // 0 - ok
        // otherwise - maximum capacity reached
        int push_front(T&& elem) {
            if (start - 1 == end) return 1;

            if (empty) {
                empty = false;
                assert(start == end);
                buffer[start] = elem;
                return 0;
            }

            if (start == 0)
                start = buffer_size - 1;
            else
                start -= 1;
            buffer[start] = {elem};

            return 0;
        }

        // 0 - ok
        // otherwise - maximum capacity reached
        int push_back(T&& elem) {
            if (end + 1 == start) return 1;

            if (empty) {
                empty = false;
                assert(start == end);
                buffer[end] = elem;
                return 0;
            }

            if (end == buffer_size - 1)
                end = 0;
            else
                end += 1;
            buffer[end] = {elem};

            return 0;
        }

        std::optional<T> pop_front() {
            if (empty) return {};

            std::optional<T> ret = buffer[start];
            buffer[start] = {};
            if (start == end) {
                empty = true;
            } else if (start == buffer_size - 1) {
                start = 0;
            } else {
                start += 1;
            }

            return ret;
        }

        std::optional<T> pop_back() {
            if (empty) return {};

            std::optional<T> ret = buffer[end];
            buffer[end] = {};
            if (start == end) {
                empty = true;
            } else if (end == 0) {
                end = buffer_size - 1;
            } else {
                end -= 1;
            }

            return ret;
        }

        T& head() {
            return *buffer[start];
        }

        T& tail() {
            return *buffer[end];
        }

        std::optional<T&> operator[](std::size_t i) {
            if (empty) {
                assert(start == end);
                return {};
            }

            i += start;
            if (i <= (end >= start) ? end : buffer_size) {
                return *buffer[i];
            }
            i -= buffer_size;

            if (i >= 0 && i <= end) {
                return *buffer[i];
            }

            return {};
        }
        std::optional<const T&> operator[](std::size_t i) const {
            return const_cast<RingBuffer*>(this)->operator[](i);
        }

        std::size_t size() const {
            if (empty) {
                assert(start == end);
                return 0;
            } else if (start <= end) {
                return end - start + 1;
            } else {
                return (end + buffer_size + 1) - start;
            }
        }

      private:
        std::vector<std::optional<T>> buffer;
        std::size_t start = 0;
        std::size_t end = 0;
        bool empty = true;
        const std::size_t buffer_size;
    };
}
