#ifndef MESHBUFFER_HPP
#define MESHBUFFER_HPP

#include "Buffer.hpp"
#include "Core/Assert.hpp"
#include "Core/UUID.hpp"
#include "GeoQikSettings.hpp"
#include "GeometryBuffers/GeometryBufferConcept.hpp"
#include <Renderer/Color.hpp>
#include "linal/linal.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace geoqik
{

struct MeshGeoBufferIndex
{
  std::size_t vertexStartIndex;
  std::size_t vertexCount;
  std::size_t triangleStartIndex;
  std::size_t triangleCount;
};

class MeshBuffer
{
  static constexpr std::int32_t m_vertexDimension = 3;
  static constexpr std::int32_t m_normalDimension = 3;
  static constexpr std::int32_t m_colorDimension = static_cast<std::int32_t>(ColorChannelCount);

  Color m_defaultMeshColor{1.0f, 1.0f, 1.0f, 1.0f};

  std::vector<float> m_vertices;
  std::vector<float> m_normals;
  std::vector<float> m_colors;
  std::vector<std::uint32_t> m_triangleIndices;

  bool m_hasChanged{false};

  std::unordered_map<core::UUID, MeshGeoBufferIndex> m_handleToMeshIndex;

  explicit MeshBuffer(const GeoQikSettings& settings)
      : m_defaultMeshColor(settings.defaultMeshColor)
  {
    m_vertices.reserve(settings.initialMeshCapacity * 3);
    m_normals.reserve(settings.initialMeshCapacity * 3);
    m_colors.reserve(settings.initialMeshCapacity * 4);
    m_triangleIndices.reserve(settings.initialMeshCapacity * 3);
  }

public:
  [[nodiscard]] static std::unique_ptr<MeshBuffer> create(const GeoQikSettings& settings)
  {
    return std::unique_ptr<MeshBuffer>(new MeshBuffer(settings));
  }

  MeshBuffer(const MeshBuffer&) = delete;
  MeshBuffer& operator=(const MeshBuffer&) = delete;
  MeshBuffer(MeshBuffer&&) = default;
  MeshBuffer& operator=(MeshBuffer&&) = default;
  ~MeshBuffer() = default;

  [[nodiscard]] bool has_changed() const { return m_hasChanged; }
  void reset_changed_flag() { m_hasChanged = false; }
  [[nodiscard]] bool empty() const { return m_vertices.empty(); }

  [[nodiscard]] Color get_default_color() const { return m_defaultMeshColor; }
  void set_default_color(float r, float g, float b, float a) { m_defaultMeshColor = {r, g, b, a}; }

  [[nodiscard]] std::span<const float> get_vertices() const { return m_vertices; }
  [[nodiscard]] std::span<const float> get_normals() const { return m_normals; }
  [[nodiscard]] std::span<const float> get_colors() const { return m_colors; }
  [[nodiscard]] std::span<const std::uint32_t> get_triangle_indices() const { return m_triangleIndices; }

  [[nodiscard]] static constexpr std::int32_t get_vertex_dimension() { return m_vertexDimension; }
  [[nodiscard]] static constexpr std::int32_t get_color_dimension() { return m_colorDimension; }

  void clear()
  {
    m_vertices.clear();
    m_normals.clear();
    m_colors.clear();
    m_triangleIndices.clear();
    m_handleToMeshIndex.clear();
    m_hasChanged = true;
  }

  void add_mesh(std::span<const float> vertices,
                std::span<const float> normals,
                std::span<const float> colors,
                std::span<const std::uint32_t> triangleIndices,
                const core::UUID* handle = nullptr)
  {
    // Validate all inputs before any mutation so that an invalid call leaves
    // the buffer unchanged (strong exception guarantee at the API level).
    if (vertices.size() % 3 != 0)
      throw std::runtime_error("MeshBuffer: vertices size must be a multiple of 3");
    if (triangleIndices.size() % 3 != 0)
      throw std::runtime_error("MeshBuffer: triangleIndices size must be a multiple of 3");
    if (handle && handle->is_nil())
      throw std::runtime_error("MeshBuffer: handle must not be nil");
    if (!normals.empty() && normals.size() != vertices.size())
      throw std::runtime_error("MeshBuffer: normals size must be 0 or equal to vertices size");

    const std::size_t vertexCount = vertices.size() / 3;
    if (!colors.empty() && colors.size() != ColorChannelCount && colors.size() != vertexCount * ColorChannelCount)
      throw std::runtime_error("MeshBuffer: colors size must be 0, 4, or vertexCount*4");

    // All validations passed — now mutate.
    const std::size_t triangleCount = triangleIndices.size() / 3;
    const std::size_t firstVertexIndex = m_vertices.size() / 3;
    const std::size_t firstTriangleIndex = m_triangleIndices.size() / 3;

    m_vertices.insert(m_vertices.end(), vertices.begin(), vertices.end());

    if (normals.empty())
    {
      compute_and_append_flat_normals(vertices, triangleIndices, vertexCount);
    }
    else
    {
      m_normals.insert(m_normals.end(), normals.begin(), normals.end());
    }

    if (colors.empty())
    {
      for (std::size_t i = 0; i < vertexCount; ++i)
        for (float c : m_defaultMeshColor) m_colors.push_back(c);
    }
    else if (colors.size() == ColorChannelCount)
    {
      for (std::size_t i = 0; i < vertexCount; ++i)
        for (std::size_t j = 0; j < ColorChannelCount; ++j) m_colors.push_back(colors[j]);
    }
    else
    {
      m_colors.insert(m_colors.end(), colors.begin(), colors.end());
    }

    for (std::uint32_t idx : triangleIndices)
      m_triangleIndices.push_back(idx + static_cast<std::uint32_t>(firstVertexIndex));

    if (handle && !handle->is_nil())
      m_handleToMeshIndex.emplace(*handle, MeshGeoBufferIndex{firstVertexIndex, vertexCount,
                                                               firstTriangleIndex, triangleCount});

    m_hasChanged = true;
  }

  void remove_mesh(core::UUID handle)
  {
    if (handle.is_nil())
      throw std::runtime_error("MeshBuffer: handle must not be nil");

    auto it = m_handleToMeshIndex.find(handle);
    if (it == m_handleToMeshIndex.end()) return;

    const std::size_t vertexStart = it->second.vertexStartIndex;
    const std::size_t vertexCount = it->second.vertexCount;
    const std::size_t triStart   = it->second.triangleStartIndex;
    const std::size_t triCount   = it->second.triangleCount;

    m_vertices.erase(m_vertices.begin() + static_cast<std::ptrdiff_t>(vertexStart * 3),
                     m_vertices.begin() + static_cast<std::ptrdiff_t>((vertexStart + vertexCount) * 3));
    m_normals.erase(m_normals.begin() + static_cast<std::ptrdiff_t>(vertexStart * 3),
                    m_normals.begin() + static_cast<std::ptrdiff_t>((vertexStart + vertexCount) * 3));
    m_colors.erase(m_colors.begin() + static_cast<std::ptrdiff_t>(vertexStart * 4),
                   m_colors.begin() + static_cast<std::ptrdiff_t>((vertexStart + vertexCount) * 4));

    m_triangleIndices.erase(m_triangleIndices.begin() + static_cast<std::ptrdiff_t>(triStart * 3),
                             m_triangleIndices.begin() + static_cast<std::ptrdiff_t>((triStart + triCount) * 3));
    const auto removedVertexCount = static_cast<std::uint32_t>(vertexCount);
    for (auto& idx : m_triangleIndices)
    {
      if (idx >= static_cast<std::uint32_t>(vertexStart + vertexCount))
        idx -= removedVertexCount;
    }

    m_handleToMeshIndex.erase(it);
    for (auto& [uuid, info] : m_handleToMeshIndex)
    {
      if (info.vertexStartIndex >= vertexStart + vertexCount)
      {
        info.vertexStartIndex -= vertexCount;
        info.triangleStartIndex -= triCount;
      }
    }

    m_hasChanged = true;
  }

  [[nodiscard]] bool update_mesh(core::UUID handle,
                                std::span<const float> vertices,
                                std::span<const float> normals,
                                std::span<const float> colors)
  {
    if (handle.is_nil()) return false;

    auto it = m_handleToMeshIndex.find(handle);
    if (it == m_handleToMeshIndex.end()) return false;

    const MeshGeoBufferIndex& info = it->second;

    // Vertex count must match exactly — topology is immutable.
    if (vertices.size() != info.vertexCount * 3) return false;
    if (!normals.empty() && normals.size() != info.vertexCount * 3) return false;
    if (!colors.empty() &&
        colors.size() != ColorChannelCount &&
        colors.size() != info.vertexCount * ColorChannelCount)
      return false;

    // Replace vertices.
    const std::size_t vBase = info.vertexStartIndex * 3;
    for (std::size_t i = 0; i < vertices.size(); ++i)
      m_vertices[vBase + i] = vertices[i];

    // Replace or recompute normals.
    if (normals.empty())
    {
      recompute_flat_normals_for_range(info.vertexStartIndex, info.vertexCount,
                                       info.triangleStartIndex, info.triangleCount);
    }
    else
    {
      const std::size_t nBase = info.vertexStartIndex * 3;
      for (std::size_t i = 0; i < normals.size(); ++i)
        m_normals[nBase + i] = normals[i];
    }

    // Replace colours if supplied.
    if (!colors.empty())
    {
      const std::size_t cBase = info.vertexStartIndex * ColorChannelCount;
      if (colors.size() == ColorChannelCount)
      {
        for (std::size_t v = 0; v < info.vertexCount; ++v)
          for (std::size_t j = 0; j < ColorChannelCount; ++j)
            m_colors[cBase + v * ColorChannelCount + j] = colors[j];
      }
      else
      {
        for (std::size_t i = 0; i < colors.size(); ++i)
          m_colors[cBase + i] = colors[i];
      }
    }

    m_hasChanged = true;
    return true;
  }

  void translate_geometry(core::UUID handle, float dx, float dy, float dz)
  {
    auto it = m_handleToMeshIndex.find(handle);
    if (it == m_handleToMeshIndex.end()) return;

    const std::size_t vStart = it->second.vertexStartIndex * 3;
    const std::size_t vEnd   = vStart + it->second.vertexCount * 3;
    for (std::size_t i = vStart; i < vEnd; i += 3)
    {
      m_vertices[i]   += dx;
      m_vertices[i+1] += dy;
      m_vertices[i+2] += dz;
    }
    m_hasChanged = true;
  }

  void rotate_geometry(core::UUID handle,
                       float centerX, float centerY, float centerZ,
                       float axisX, float axisY, float axisZ,
                       float angle)
  {
    auto it = m_handleToMeshIndex.find(handle);
    if (it == m_handleToMeshIndex.end()) return;

    linal::float3 center{centerX, centerY, centerZ};
    linal::float3 axis{axisX, axisY, axisZ};
    axis = axis.normalize();

    linal::float33 rotationMatrix;
    linal::rot_axis(rotationMatrix, axis, angle);

    const std::size_t vStart = it->second.vertexStartIndex * 3;
    const std::size_t vEnd   = vStart + it->second.vertexCount * 3;
    for (std::size_t i = vStart; i < vEnd; i += 3)
    {
      linal::float3 point{m_vertices[i], m_vertices[i+1], m_vertices[i+2]};
      linal::float3 rotated = rotationMatrix * (point - center) + center;
      m_vertices[i]   = rotated[0];
      m_vertices[i+1] = rotated[1];
      m_vertices[i+2] = rotated[2];
    }

    recompute_flat_normals_for_range(it->second.vertexStartIndex, it->second.vertexCount,
                                     it->second.triangleStartIndex, it->second.triangleCount);
    m_hasChanged = true;
  }

private:
  void compute_and_append_flat_normals(std::span<const float> vertices,
                                       std::span<const std::uint32_t> triangleIndices,
                                       std::size_t vertexCount)
  {
    std::vector<float> normals(vertexCount * 3, 0.0f);
    for (std::size_t t = 0; t < triangleIndices.size() / 3; ++t)
    {
      const std::uint32_t i0 = triangleIndices[t * 3 + 0];
      const std::uint32_t i1 = triangleIndices[t * 3 + 1];
      const std::uint32_t i2 = triangleIndices[t * 3 + 2];
      const linal::float3 v0{vertices[i0*3], vertices[i0*3+1], vertices[i0*3+2]};
      const linal::float3 v1{vertices[i1*3], vertices[i1*3+1], vertices[i1*3+2]};
      const linal::float3 v2{vertices[i2*3], vertices[i2*3+1], vertices[i2*3+2]};
      const linal::float3 normal = linal::normalize(linal::cross(v1 - v0, v2 - v0));
      for (const std::size_t k : {static_cast<std::size_t>(i0), static_cast<std::size_t>(i1), static_cast<std::size_t>(i2)})
      {
        normals[k*3]   = normal[0];
        normals[k*3+1] = normal[1];
        normals[k*3+2] = normal[2];
      }
    }
    m_normals.insert(m_normals.end(), normals.begin(), normals.end());
  }

  void recompute_flat_normals_for_range(std::size_t vertexStartIndex, std::size_t vertexCount,
                                         std::size_t triangleStartIndex, std::size_t triangleCount)
  {
    for (std::size_t t = triangleStartIndex; t < triangleStartIndex + triangleCount; ++t)
    {
      const std::uint32_t gi0 = m_triangleIndices[t * 3 + 0];
      const std::uint32_t gi1 = m_triangleIndices[t * 3 + 1];
      const std::uint32_t gi2 = m_triangleIndices[t * 3 + 2];
      const linal::float3 v0{m_vertices[gi0*3], m_vertices[gi0*3+1], m_vertices[gi0*3+2]};
      const linal::float3 v1{m_vertices[gi1*3], m_vertices[gi1*3+1], m_vertices[gi1*3+2]};
      const linal::float3 v2{m_vertices[gi2*3], m_vertices[gi2*3+1], m_vertices[gi2*3+2]};
      const linal::float3 normal = linal::normalize(linal::cross(v1 - v0, v2 - v0));
      for (const std::uint32_t k : {gi0, gi1, gi2})
      {
        m_normals[k*3]   = normal[0];
        m_normals[k*3+1] = normal[1];
        m_normals[k*3+2] = normal[2];
      }
    }
    // suppress unused variable warning when triangleCount==0
    static_cast<void>(vertexStartIndex);
    static_cast<void>(vertexCount);
  }
};

static_assert(GeometryBuffer<MeshBuffer>);

} // namespace geoqik

#endif // MESHBUFFER_HPP
