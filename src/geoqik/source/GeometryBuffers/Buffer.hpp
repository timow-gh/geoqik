#ifndef GEOQIK_GEOMETRYBUFFERS_BUFFER_HPP
#define GEOQIK_GEOMETRYBUFFERS_BUFFER_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <span>
#include <utility>
#include <vector>

namespace opengl {

// Plinth v2 no longer exposes its former CPU-side Buffer helper.  GeoQik uses
// this only to stage geometry before sending spans to Plinth, so keeping the
// storage local avoids depending on a renderer-internal container.
template <typename T>
class Buffer {
  public:
    Buffer() = default;

    explicit Buffer(std::size_t capacity)
        : m_capacity(capacity) {
        m_data.reserve(capacity);
    }

    [[nodiscard]] static Buffer create_from(const Buffer& other, std::size_t capacity) {
        Buffer result(std::max(capacity, other.size()));
        result.m_data = other.m_data;
        return result;
    }

    [[nodiscard]] bool is_empty() const noexcept { return m_data.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return m_data.size(); }
    [[nodiscard]] std::size_t capacity() const noexcept { return m_capacity; }
    [[nodiscard]] std::size_t free_capacity() const noexcept { return m_capacity - m_data.size(); }

    void reset() noexcept { m_data.clear(); }

    void push_back(const T& value) {
        assert(m_data.size() < m_capacity);
        m_data.push_back(value);
    }

    void push_back(T&& value) {
        assert(m_data.size() < m_capacity);
        m_data.push_back(std::move(value));
    }

    void remove(std::size_t index, std::size_t count) {
        assert(index <= m_data.size());
        assert(count <= m_data.size() - index);
        m_data.erase(m_data.begin() + static_cast<std::ptrdiff_t>(index),
                     m_data.begin() + static_cast<std::ptrdiff_t>(index + count));
    }

    [[nodiscard]] std::span<const T> get_as_span() const noexcept { return m_data; }

    [[nodiscard]] T& operator[](std::size_t index) noexcept { return m_data[index]; }
    [[nodiscard]] const T& operator[](std::size_t index) const noexcept { return m_data[index]; }

    [[nodiscard]] auto begin() noexcept { return m_data.begin(); }
    [[nodiscard]] auto end() noexcept { return m_data.end(); }
    [[nodiscard]] auto begin() const noexcept { return m_data.begin(); }
    [[nodiscard]] auto end() const noexcept { return m_data.end(); }

  private:
    std::size_t m_capacity{0};
    std::vector<T> m_data;
};

} // namespace opengl

#endif // GEOQIK_GEOMETRYBUFFERS_BUFFER_HPP
