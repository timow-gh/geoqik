#ifndef POINTBUFFER_HPP
#define POINTBUFFER_HPP

#include "../GeoQikSettings.hpp"
#include "Buffer.hpp"
#include "Core/Assert.hpp"
#include "Core/UUID.hpp"
#include "linal/linal.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace geoqik {

// Holds the index of a point in the point buffer
struct PointGeoBufferIndex {
    std::size_t pointIndex;

    [[nodiscard]]
    std::strong_ordering operator<=>(const PointGeoBufferIndex& other) const = default;
};

struct PointsGeoBufferIndex {
    std::size_t pointStartIndex;
    std::size_t pointEndIndex; // inclusive, this index is part of the range.

    [[nodiscard]]
    std::strong_ordering operator<=>(const PointsGeoBufferIndex& other) const = default;
};

struct PointBufferSnapshot {
    Color currentPointColor;
    std::vector<float> points;
    std::vector<float> pointColors;
    std::vector<std::uint32_t> pointIndices;
    std::size_t pointCapacity{0};
    std::size_t pointColorCapacity{0};
    std::size_t pointIndexCapacity{0};
    std::unordered_map<core::UUID, PointGeoBufferIndex> handleToPointIndexMapping;
    std::unordered_map<core::UUID, PointsGeoBufferIndex> handleToPointsIndexMapping;
};

struct PointBufferGeometry {
    std::vector<float> points;
    std::vector<float> colors;
};

class PointBuffer {
    static constexpr std::int32_t m_pointDimension = 3;                                            // x, y, z
    static constexpr std::int32_t m_colorDimension = static_cast<std::int32_t>(ColorChannelCount); // r, g, b, a

    Color m_currentPointColor{1.0f, 1.0f, 1.0f, 1.0f};
    Buffer<float> m_points;
    Buffer<float> m_pointColors;
    Buffer<std::uint32_t> m_pointIndices;

    bool m_pointsHaveChanged{false};

    std::unordered_map<core::UUID, PointGeoBufferIndex> m_handleToPointIndexMapping;
    std::unordered_map<core::UUID, PointsGeoBufferIndex> m_handleToPointsIndexMapping;

  public:
    [[nodiscard]]
    static std::unique_ptr<PointBuffer> create() {
        return std::unique_ptr<PointBuffer>(new PointBuffer());
    }

    [[nodiscard]]
    static std::unique_ptr<PointBuffer> create(const GeoQikSettings& settings) {
        return std::unique_ptr<PointBuffer>(new PointBuffer(settings));
    }

    [[nodiscard]]
    static std::unique_ptr<PointBuffer> create_from(const PointBuffer& other, std::size_t growthFactor) {
        auto newBuffer = PointBuffer::create();

        newBuffer->m_currentPointColor = other.m_currentPointColor;
        if (!other.m_points.is_empty()) {
            newBuffer->m_points = Buffer<float>::create_from(other.m_points, other.m_points.capacity() * growthFactor);
            newBuffer->m_pointColors =
                Buffer<float>::create_from(other.m_pointColors, other.m_pointColors.capacity() * growthFactor);
            newBuffer->m_pointIndices =
                Buffer<std::uint32_t>::create_from(other.m_pointIndices,
                                                   other.m_pointIndices.capacity() * growthFactor);
            newBuffer->m_handleToPointIndexMapping = std::move(other.m_handleToPointIndexMapping);
            newBuffer->m_handleToPointsIndexMapping = std::move(other.m_handleToPointsIndexMapping);
            newBuffer->m_pointsHaveChanged = true;
        }

        return newBuffer;
    }

    PointBuffer(const PointBuffer&) = delete;
    PointBuffer& operator=(const PointBuffer&) = delete;
    PointBuffer(PointBuffer&&) = default;
    PointBuffer& operator=(PointBuffer&&) = default;
    ~PointBuffer() = default;

    [[nodiscard]]
    bool has_changed() const {
        return m_pointsHaveChanged;
    }
    void reset_changed_flag() { m_pointsHaveChanged = false; }

    [[nodiscard]]
    bool points_have_changed() const {
        return m_pointsHaveChanged;
    }
    void reset_points_have_changed() { m_pointsHaveChanged = false; }

    [[nodiscard]]
    bool empty() const {
        return m_points.is_empty();
    }

    void clear() {
        m_points.reset();
        m_pointColors.reset();
        m_pointIndices.reset();
        m_handleToPointIndexMapping.clear();
        m_handleToPointsIndexMapping.clear();
        m_pointsHaveChanged = true;
    }

    [[nodiscard]]
    static constexpr std::int32_t get_point_dimension() {
        return m_pointDimension;
    }
    [[nodiscard]]
    static constexpr std::int32_t get_color_dimension() {
        return m_colorDimension;
    }

    [[nodiscard]]
    Color get_default_point_color() const {
        return m_currentPointColor;
    }

    [[nodiscard]]
    std::optional<PointBufferGeometry> get_geometry(core::UUID handle) const {
        if (auto it = m_handleToPointIndexMapping.find(handle); it != m_handleToPointIndexMapping.end()) {
            const std::size_t pointIndex = it->second.pointIndex;
            return create_geometry(pointIndex, pointIndex);
        }

        if (auto it = m_handleToPointsIndexMapping.find(handle); it != m_handleToPointsIndexMapping.end()) {
            return create_geometry(it->second.pointStartIndex, it->second.pointEndIndex);
        }

        return std::nullopt;
    }

    [[nodiscard]]
    PointBufferSnapshot create_snapshot() const {
        PointBufferSnapshot snapshot;
        snapshot.currentPointColor = m_currentPointColor;
        snapshot.points.assign(m_points.begin(), m_points.end());
        snapshot.pointColors.assign(m_pointColors.begin(), m_pointColors.end());
        snapshot.pointIndices.assign(m_pointIndices.begin(), m_pointIndices.end());
        snapshot.pointCapacity = m_points.capacity();
        snapshot.pointColorCapacity = m_pointColors.capacity();
        snapshot.pointIndexCapacity = m_pointIndices.capacity();
        snapshot.handleToPointIndexMapping = m_handleToPointIndexMapping;
        snapshot.handleToPointsIndexMapping = m_handleToPointsIndexMapping;
        return snapshot;
    }

    void restore_snapshot(const PointBufferSnapshot& snapshot) {
        m_currentPointColor = snapshot.currentPointColor;
        m_points = Buffer<float>(std::max(snapshot.pointCapacity, snapshot.points.size()));
        m_pointColors = Buffer<float>(std::max(snapshot.pointColorCapacity, snapshot.pointColors.size()));
        m_pointIndices = Buffer<std::uint32_t>(std::max(snapshot.pointIndexCapacity, snapshot.pointIndices.size()));

        for (float point: snapshot.points) {
            m_points.push_back(point);
        }
        for (float color: snapshot.pointColors) {
            m_pointColors.push_back(color);
        }
        for (std::uint32_t index: snapshot.pointIndices) {
            m_pointIndices.push_back(index);
        }

        m_handleToPointIndexMapping = snapshot.handleToPointIndexMapping;
        m_handleToPointsIndexMapping = snapshot.handleToPointsIndexMapping;
        m_pointsHaveChanged = true;
    }

    // clang-format off
  [[nodiscard]] std::span<const float> get_points() const { return m_points.get_as_span(); }
  [[nodiscard]] std::span<const float> get_point_colors() const { return m_pointColors.get_as_span(); }
  [[nodiscard]] std::span<const std::uint32_t> get_point_indices() const { return m_pointIndices.get_as_span(); }
    // clang-format on

    [[nodiscard]]
    std::size_t get_point_capacity() const {
        return m_points.capacity() / m_pointDimension;
    }
    [[nodiscard]]
    std::size_t get_free_point_capacity() const {
        return m_points.free_capacity() / m_pointDimension;
    }

    bool has_space_for_points(std::size_t count) const { return get_free_point_capacity() >= count; }

    void add_point(float x, float y, float z, const core::UUID* handle = nullptr) {
        add_point(x,
                  y,
                  z,
                  m_currentPointColor[0],
                  m_currentPointColor[1],
                  m_currentPointColor[2],
                  m_currentPointColor[3],
                  handle);
    }

    void add_point(float x, float y, float z, float r, float g, float b, float a, const core::UUID* handle = nullptr) {
        if (handle && handle->is_nil()) {
            throw std::runtime_error("GeometryBuffer: Attempting to add a point with a nil handle");
        }
        if (has_space_for_points(1) == false) {
            throw std::runtime_error("GeometryBuffer: Not enough space for points");
        }

        std::size_t currentPointIndex = m_points.size() / m_pointDimension;

        m_points.push_back(x);
        m_points.push_back(y);
        m_points.push_back(z);

        m_pointColors.push_back(r);
        m_pointColors.push_back(g);
        m_pointColors.push_back(b);
        m_pointColors.push_back(a);

        CORE_ASSERT(currentPointIndex <= static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()));
        m_pointIndices.push_back(static_cast<std::uint32_t>(currentPointIndex));

        if (handle != nullptr && !handle->is_nil()) {
            m_handleToPointIndexMapping.emplace(*handle, PointGeoBufferIndex{currentPointIndex});
        }

        m_pointsHaveChanged = true;
    }

    void remove_point(core::UUID handle) {
        if (handle.is_nil()) {
            throw std::runtime_error("GeometryBuffer: Attempting to remove a point with a nil handle");
        }

        if (remove_single_point(handle)) {
            return;
        }

        if (remove_points(handle)) {
            return;
        }
    }

    [[nodiscard]]
    bool update_point(core::UUID handle, float x, float y, float z, std::span<const float> colors = {}) {
        if (handle.is_nil()) {
            return false;
        }

        auto it = m_handleToPointIndexMapping.find(handle);
        if (it == m_handleToPointIndexMapping.end()) {
            return false;
        }

        const std::size_t pointIndex = it->second.pointIndex;
        if (!colors.empty() && colors.size() != ColorChannelCount) {
            return false;
        }

        const std::size_t pointStart = pointIndex * m_pointDimension;
        m_points[pointStart] = x;
        m_points[pointStart + 1] = y;
        m_points[pointStart + 2] = z;

        if (!colors.empty()) {
            const std::size_t colorStart = pointIndex * m_colorDimension;
            for (std::size_t colorIndex = 0; colorIndex < ColorChannelCount; ++colorIndex) {
                m_pointColors[colorStart + colorIndex] = colors[colorIndex];
            }
        }

        m_pointsHaveChanged = true;
        return true;
    }

    [[nodiscard]]
    bool update_points(core::UUID handle, std::span<const float> points, std::span<const float> colors = {}) {
        if (handle.is_nil()) {
            return false;
        }
        if (points.size() % m_pointDimension != 0) {
            throw std::runtime_error(
                "GeometryBuffer: The size of the points span must be a multiple of the point dimension.");
        }

        if (auto pointIt = m_handleToPointIndexMapping.find(handle); pointIt != m_handleToPointIndexMapping.end()) {
            if (points.size() != m_pointDimension) {
                return false;
            }
            return update_point(handle, points[0], points[1], points[2], colors);
        }

        auto pointsIt = m_handleToPointsIndexMapping.find(handle);
        if (pointsIt == m_handleToPointsIndexMapping.end()) {
            return false;
        }

        const std::size_t pointStartIndex = pointsIt->second.pointStartIndex;
        const std::size_t pointEndIndex = pointsIt->second.pointEndIndex;
        const std::size_t pointCount = pointEndIndex - pointStartIndex + 1;
        if (points.size() != pointCount * m_pointDimension) {
            return false;
        }
        if (!colors.empty() && colors.size() != ColorChannelCount && colors.size() != pointCount * ColorChannelCount) {
            return false;
        }

        const std::size_t pointStart = pointStartIndex * m_pointDimension;
        for (std::size_t i = 0; i < points.size(); ++i) {
            m_points[pointStart + i] = points[i];
        }

        if (colors.size() == ColorChannelCount) {
            for (std::size_t pointIndex = pointStartIndex; pointIndex <= pointEndIndex; ++pointIndex) {
                const std::size_t colorStart = pointIndex * m_colorDimension;
                for (std::size_t colorIndex = 0; colorIndex < ColorChannelCount; ++colorIndex) {
                    m_pointColors[colorStart + colorIndex] = colors[colorIndex];
                }
            }
        } else if (colors.size() == pointCount * ColorChannelCount) {
            const std::size_t colorStart = pointStartIndex * m_colorDimension;
            for (std::size_t i = 0; i < colors.size(); ++i) {
                m_pointColors[colorStart + i] = colors[i];
            }
        }

        m_pointsHaveChanged = true;
        return true;
    }

    void add_points(std::span<const float> points, std::span<const float> colors, const core::UUID* handle = nullptr) {
        if (points.empty() && colors.empty()) {
            return;
        }
        if (handle && handle->is_nil()) {
            throw std::runtime_error("GeometryBuffer: Attempting to add points with a nil handle");
        }
        if (points.size() % m_pointDimension != 0) {
            throw std::runtime_error(
                "GeometryBuffer: The size of the points span must be a multiple of the point dimension.");
        }
        std::size_t pointCount = points.size() / m_pointDimension;
        if (has_space_for_points(pointCount) == false) {
            throw std::runtime_error("GeometryBuffer: Not enough space for points");
        }

        std::size_t currentPointIndex = m_points.size() / m_pointDimension;

        if (colors.size() == 0) {
            for (std::size_t i = 0; i < pointCount; ++i) {
                const std::size_t pointBase = i * m_pointDimension;
                m_points.push_back(points[pointBase]);
                m_points.push_back(points[pointBase + 1]);
                m_points.push_back(points[pointBase + 2]);
                for (float channel: m_currentPointColor) {
                    m_pointColors.push_back(channel);
                }
            }
        } else if (colors.size() == ColorChannelCount && colors[0] >= 0.0f && colors[1] >= 0.0f && colors[2] >= 0.0f &&
                   colors[3] >= 0.0f) {
            for (std::size_t i = 0; i < pointCount; ++i) {
                const std::size_t pointBase = i * m_pointDimension;
                m_points.push_back(points[pointBase]);
                m_points.push_back(points[pointBase + 1]);
                m_points.push_back(points[pointBase + 2]);
                for (std::size_t colorIndex = 0; colorIndex < ColorChannelCount; ++colorIndex) {
                    m_pointColors.push_back(colors[colorIndex]);
                }
            }
        } else if (colors.size() == pointCount * ColorChannelCount) {
            for (std::size_t i = 0; i < pointCount; ++i) {
                const std::size_t pointBase = i * m_pointDimension;
                const std::size_t colorBase = i * ColorChannelCount;
                m_points.push_back(points[pointBase]);
                m_points.push_back(points[pointBase + 1]);
                m_points.push_back(points[pointBase + 2]);
                for (std::size_t colorIndex = 0; colorIndex < ColorChannelCount; ++colorIndex) {
                    m_pointColors.push_back(colors[colorBase + colorIndex]);
                }
            }
        } else {
            throw std::runtime_error(
                "GeometryBuffer: Invalid colors span. It must be either empty, have a size of 4 with valid RGBA "
                "values, or match pointCount * 4.");
        }

        for (std::uint32_t i = static_cast<std::uint32_t>(currentPointIndex);
             i < static_cast<std::uint32_t>(currentPointIndex + pointCount);
             ++i) {
            m_pointIndices.push_back(i);
        }

        if (handle) {
            m_handleToPointsIndexMapping.emplace(
                *handle,
                PointsGeoBufferIndex{currentPointIndex, currentPointIndex + pointCount - 1});
        }

        m_pointsHaveChanged = true;
    }

    void set_default_point_color(float r, float g, float b, float a) { m_currentPointColor = {r, g, b, a}; }

    void translate_geometry(core::UUID handle, float dx, float dy, float dz) {
        auto pointIt = m_handleToPointIndexMapping.find(handle);
        if (pointIt != m_handleToPointIndexMapping.end()) {
            std::size_t pointIndex = pointIt->second.pointIndex;
            std::size_t pointStart = pointIndex * m_pointDimension;
            m_points[pointStart] += dx;
            m_points[pointStart + 1] += dy;
            m_points[pointStart + 2] += dz;
            m_pointsHaveChanged = true;
            return;
        }

        auto pointsIt = m_handleToPointsIndexMapping.find(handle);
        if (pointsIt != m_handleToPointsIndexMapping.end()) {
            translate_point_range(pointsIt->second.pointStartIndex, pointsIt->second.pointEndIndex, dx, dy, dz);
        }
    }

    void rotate_geometry(core::UUID handle,
                         float centerX,
                         float centerY,
                         float centerZ,
                         float axisX,
                         float axisY,
                         float axisZ,
                         float angle) {
        auto pointIt = m_handleToPointIndexMapping.find(handle);
        if (pointIt != m_handleToPointIndexMapping.end()) {
            std::size_t pointIndex = pointIt->second.pointIndex;
            std::size_t pointStart = pointIndex * m_pointDimension;

            linal::float3 point{m_points[pointStart], m_points[pointStart + 1], m_points[pointStart + 2]};
            linal::float3 center{centerX, centerY, centerZ};
            linal::float3 axis{axisX, axisY, axisZ};
            axis = axis.normalize();

            linal::float33 rotationMatrix;
            linal::rot_axis(rotationMatrix, axis, angle);

            linal::float3 rotatedPoint = rotationMatrix * (point - center) + center;

            m_points[pointStart] = rotatedPoint[0];
            m_points[pointStart + 1] = rotatedPoint[1];
            m_points[pointStart + 2] = rotatedPoint[2];
            m_pointsHaveChanged = true;
            return;
        }

        auto pointsIt = m_handleToPointsIndexMapping.find(handle);
        if (pointsIt != m_handleToPointsIndexMapping.end()) {
            rotate_point_range(pointsIt->second.pointStartIndex,
                               pointsIt->second.pointEndIndex,
                               centerX,
                               centerY,
                               centerZ,
                               axisX,
                               axisY,
                               axisZ,
                               angle);
        }
    }

  private:
    PointBuffer()
        : PointBuffer(GeoQikSettings{}) {}

    PointBuffer(const GeoQikSettings& settings)
        : m_currentPointColor(settings.defaultPointColor)
        , m_points(settings.initialPointCapacity * m_pointDimension)
        , m_pointColors(settings.initialPointCapacity * m_colorDimension)
        , m_pointIndices(settings.initialPointCapacity) {
        assert(m_points.capacity() % m_pointDimension == 0);
        assert(m_pointColors.capacity() % m_colorDimension == 0);
        assert(m_pointIndices.capacity() == settings.initialPointCapacity);
    }

    [[nodiscard]]
    PointBufferGeometry create_geometry(std::size_t pointStartIndex, std::size_t pointEndIndex) const {
        PointBufferGeometry geometry;
        const std::size_t pointCount = pointEndIndex - pointStartIndex + 1;
        const std::size_t pointStart = pointStartIndex * m_pointDimension;
        const std::size_t colorStart = pointStartIndex * m_colorDimension;
        geometry.points.assign(m_points.begin() + pointStart,
                               m_points.begin() + pointStart + pointCount * m_pointDimension);
        geometry.colors.assign(m_pointColors.begin() + colorStart,
                               m_pointColors.begin() + colorStart + pointCount * m_colorDimension);
        return geometry;
    }

    void translate_point_range(std::size_t pointStartIndex, std::size_t pointEndIndex, float dx, float dy, float dz) {
        for (std::size_t pointIndex = pointStartIndex; pointIndex <= pointEndIndex; ++pointIndex) {
            const std::size_t pointStart = pointIndex * m_pointDimension;
            m_points[pointStart] += dx;
            m_points[pointStart + 1] += dy;
            m_points[pointStart + 2] += dz;
        }
        m_pointsHaveChanged = true;
    }

    void rotate_point_range(std::size_t pointStartIndex,
                            std::size_t pointEndIndex,
                            float centerX,
                            float centerY,
                            float centerZ,
                            float axisX,
                            float axisY,
                            float axisZ,
                            float angle) {
        linal::float3 center{centerX, centerY, centerZ};
        linal::float3 axis{axisX, axisY, axisZ};
        axis = axis.normalize();

        linal::float33 rotationMatrix;
        linal::rot_axis(rotationMatrix, axis, angle);

        for (std::size_t pointIndex = pointStartIndex; pointIndex <= pointEndIndex; ++pointIndex) {
            const std::size_t pointStart = pointIndex * m_pointDimension;
            linal::float3 point{m_points[pointStart], m_points[pointStart + 1], m_points[pointStart + 2]};
            linal::float3 rotatedPoint = rotationMatrix * (point - center) + center;
            m_points[pointStart] = rotatedPoint[0];
            m_points[pointStart + 1] = rotatedPoint[1];
            m_points[pointStart + 2] = rotatedPoint[2];
        }
        m_pointsHaveChanged = true;
    }

    bool remove_single_point(core::UUID handle) {
        auto it = m_handleToPointIndexMapping.find(handle);
        if (it == m_handleToPointIndexMapping.end()) {
            return false;
        }

        // Iterate the existing indices and decrement those greater than the removed point index.
        // Otherwise they would point to the wrong data after removal of the current point.
        std::size_t pointIndex = it->second.pointIndex;
        for (auto& pairIter: m_handleToPointIndexMapping) {
            if (pointIndex < pairIter.second.pointIndex) {
                pairIter.second.pointIndex--;
            }
        }
        for (auto& pairIter: m_handleToPointsIndexMapping) {
            if (pointIndex < pairIter.second.pointStartIndex) {
                pairIter.second.pointStartIndex--;
                pairIter.second.pointEndIndex--;
            }
        }

        std::size_t pointStart = pointIndex * m_pointDimension;
        std::size_t colorStart = pointIndex * m_colorDimension;
        m_points.remove(pointStart, m_pointDimension);
        m_pointColors.remove(colorStart, m_colorDimension);

        // Fix the indices in the index buffer and remove the index of the removed point.
        // The indices after the removed point need to be decremented by one.
        for (std::size_t i = 0; i < m_pointIndices.size(); ++i) {
            if (m_pointIndices[i] > pointIndex) {
                m_pointIndices[i]--;
            }
        }
        m_pointIndices.remove(pointIndex, 1);
        m_handleToPointIndexMapping.erase(handle);

        m_pointsHaveChanged = true;

        return true;
    }

    bool remove_points(core::UUID handle) {
        if (handle.is_nil()) {
            throw std::runtime_error("GeometryBuffer: Attempting to remove points with a nil handle");
        }

        auto it = m_handleToPointsIndexMapping.find(handle);
        if (it == m_handleToPointsIndexMapping.end()) {
            return false;
        }

        std::size_t pointStartIndex = it->second.pointStartIndex;
        std::size_t pointEndIndex = it->second.pointEndIndex;

        std::size_t numberOfPointsToRemove = pointEndIndex - pointStartIndex + 1;
        std::size_t pointStart = pointStartIndex * m_pointDimension;
        std::size_t colorStart = pointStartIndex * m_colorDimension;
        m_points.remove(pointStart, numberOfPointsToRemove * m_pointDimension);
        m_pointColors.remove(colorStart, numberOfPointsToRemove * m_colorDimension);

        // Fix the indices in the index buffer and remove the indices of the removed points.
        // The indices after the removed points need to be decremented by the number of removed points.
        for (std::size_t i = 0; i < m_pointIndices.size(); ++i) {
            if (m_pointIndices[i] > pointEndIndex) {
                m_pointIndices[i] -= static_cast<std::uint32_t>(numberOfPointsToRemove);
            }
        }
        m_pointIndices.remove(pointStartIndex, numberOfPointsToRemove);

        // Iterate existing indices in the handle to points mapping and decrement those greater than the removed points.
        // Assume the ranges of points do not overlap. This is ensured by the GeometryBuffer.
        for (auto& pairIter: m_handleToPointsIndexMapping) {
            if (pointEndIndex < pairIter.second.pointStartIndex) {
                pairIter.second.pointStartIndex -= numberOfPointsToRemove;
                pairIter.second.pointEndIndex -= numberOfPointsToRemove;
            }
        }
        for (auto& pairIter: m_handleToPointIndexMapping) {
            if (pointEndIndex < pairIter.second.pointIndex) {
                pairIter.second.pointIndex -= numberOfPointsToRemove;
            }
        }
        m_handleToPointsIndexMapping.erase(handle);

        m_pointsHaveChanged = true;

        return true;
    }
};

} // namespace geoqik

#endif // POINTBUFFER_HPP
