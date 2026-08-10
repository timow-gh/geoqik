#ifndef GEOQIK_SOURCE_SCENE_HPP
#define GEOQIK_SOURCE_SCENE_HPP

#include "GeoQikSettings.hpp"
#include "GeometryBuffers/GeometryStyle.hpp"
#include "GeometryBuffers/LineBuffer.hpp"
#include "GeometryBuffers/MeshBuffer.hpp"
#include "GeometryBuffers/PointBuffer.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>

namespace geoqik {

struct SceneSnapshot {
    PointBufferSnapshot pointBuffer;
    LineBufferSnapshot lineBuffer;
    MeshBufferSnapshot meshBuffer;
    float pointSize{3.0f};
    float lineWidth{1.0f};
    std::unordered_map<core::UUID, StyledPointData> styledPoints;
    std::unordered_map<core::UUID, StyledLineData> styledLines;
};

struct BoundingSphere {
    linal::float3 center{0.0f, 0.0f, 0.0f};
    float radius{0.0f};

    [[nodiscard]] float get_radius() const { return radius; }
    [[nodiscard]] const linal::float3& get_center() const { return center; }
};

class Scene {
    std::unique_ptr<PointBuffer> m_pointBuffer;
    std::unique_ptr<LineBuffer> m_lineBuffer;
    std::unique_ptr<MeshBuffer> m_meshBuffer;
    std::size_t m_geomBufferGrowthFactor{2};
    float m_pointSize{3.0f};
    float m_lineWidth{1.0f};
    std::unordered_map<core::UUID, StyledPointData> m_styledPoints;
    std::unordered_map<core::UUID, StyledLineData> m_styledLines;
    bool m_styledDirty{false};

  public:
    Scene() = default;
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) noexcept = default;
    Scene& operator=(Scene&&) noexcept = default;
    ~Scene() = default;

    [[nodiscard]] static Scene create(const GeoQikSettings& geoqikSettings);

    [[nodiscard]] bool ensure_point_capacity(std::size_t pointCount);
    [[nodiscard]] bool ensure_line_capacity(std::size_t lineCount);

    void add_point(float x, float y, float z, const core::UUID* handle = nullptr);
    void add_point(float x, float y, float z, float r, float g, float b, float a, const core::UUID* handle = nullptr);
    void add_points(std::span<const float> points, std::span<const float> colors, const core::UUID* handle = nullptr);
    [[nodiscard]] bool update_point(core::UUID handle, float x, float y, float z, std::span<const float> colors = {});
    [[nodiscard]] bool
    update_points(core::UUID handle, std::span<const float> points, std::span<const float> colors = {});
    void remove_point(core::UUID handle);
    void add_styled_points(core::UUID handle, StyledPointData data);
    [[nodiscard]] bool update_styled_points(core::UUID handle,
                                            std::span<const float> points,
                                            std::span<const float> colors,
                                            std::span<const float> radii,
                                            std::uint8_t sizeSpace,
                                            bool restyle);

    void add_line(float x1, float y1, float z1, float x2, float y2, float z2, const core::UUID* handle = nullptr);
    void add_line(float x1,
                  float y1,
                  float z1,
                  float x2,
                  float y2,
                  float z2,
                  float r,
                  float g,
                  float b,
                  float a,
                  const core::UUID* handle = nullptr);
    void add_lines(std::span<const float> lines, std::span<const float> colors, const core::UUID* handle = nullptr);
    [[nodiscard]] bool update_line(core::UUID handle,
                                   float x1,
                                   float y1,
                                   float z1,
                                   float x2,
                                   float y2,
                                   float z2,
                                   std::span<const float> colors = {});
    [[nodiscard]] bool
    update_lines(core::UUID handle, std::span<const float> lines, std::span<const float> colors = {});
    void remove_line(core::UUID handle);
    void add_styled_line(core::UUID handle, StyledLineData data);
    [[nodiscard]] bool update_styled_line(core::UUID handle,
                                          std::span<const float> vertices,
                                          std::span<const float> colors,
                                          const StrokeStyleData& style,
                                          std::uint8_t lineType,
                                          std::span<const std::uint8_t> perVertexDashFlags,
                                          bool restyle);

    void add_mesh(std::span<const float> vertices,
                  std::span<const float> normals,
                  std::span<const float> colors,
                  std::span<const std::uint32_t> triangleIndices,
                  const core::UUID* handle = nullptr);
    void remove_mesh(core::UUID handle);
    [[nodiscard]] bool update_mesh(core::UUID handle,
                                   std::span<const float> vertices,
                                   std::span<const float> normals,
                                   std::span<const float> colors);

    // Toggle overlay visibility for an existing mesh.
    void set_mesh_overlay_opts(core::UUID handle, bool showSegments, bool showVertices);

    // Apply per-mesh rendering options (triggers drawable rebuild).
    void set_mesh_rendering_opts(core::UUID handle, PerMeshRenderingOpts opts);

    [[nodiscard]] const MeshBuffer& get_mesh_buffer() const { return *m_meshBuffer; }
    [[nodiscard]] MeshBuffer& get_mesh_buffer() { return *m_meshBuffer; }

    void translate_geometry(core::UUID handle, float dx, float dy, float dz);
    void rotate_geometry(core::UUID handle,
                         float centerX,
                         float centerY,
                         float centerZ,
                         float axisX,
                         float axisY,
                         float axisZ,
                         float angle);
    void scale_geometry(core::UUID handle, float cx, float cy, float cz, float sx, float sy, float sz);
    void set_geometry_color(core::UUID handle, float r, float g, float b, float a);

    void clear();

    [[nodiscard]] SceneSnapshot create_snapshot() const;
    void restore_snapshot(const SceneSnapshot& snapshot);
    [[nodiscard]] std::optional<PointBufferGeometry> get_point_geometry(core::UUID handle) const;
    [[nodiscard]] std::optional<LineBufferGeometry> get_line_geometry(core::UUID handle) const;
    [[nodiscard]] bool is_styled(core::UUID handle) const;
    [[nodiscard]] bool is_styled_point(core::UUID handle) const { return m_styledPoints.contains(handle); }
    [[nodiscard]] bool is_styled_line(core::UUID handle) const { return m_styledLines.contains(handle); }
    [[nodiscard]] const std::unordered_map<core::UUID, StyledPointData>& get_styled_points() const {
        return m_styledPoints;
    }
    [[nodiscard]] const std::unordered_map<core::UUID, StyledLineData>& get_styled_lines() const {
        return m_styledLines;
    }
    [[nodiscard]] bool styled_dirty() const { return m_styledDirty; }
    void reset_styled_dirty() { m_styledDirty = false; }

    [[nodiscard]] float get_point_size() const { return m_pointSize; }
    void set_point_size(float pointSize) { m_pointSize = pointSize; }

    [[nodiscard]] Color get_default_point_color() const;
    void set_default_point_color(float r, float g, float b, float a);

    [[nodiscard]] float get_line_width() const { return m_lineWidth; }
    void set_line_width(float lineWidth) { m_lineWidth = lineWidth; }

    [[nodiscard]] Color get_default_line_color() const;
    void set_default_line_color(float r, float g, float b, float a);

    [[nodiscard]] Color get_default_mesh_color() const;
    void set_default_mesh_color(float r, float g, float b, float a);

    [[nodiscard]] const PointBuffer& get_point_buffer() const { return *m_pointBuffer; }
    [[nodiscard]] PointBuffer& get_point_buffer() { return *m_pointBuffer; }
    [[nodiscard]] const LineBuffer& get_line_buffer() const { return *m_lineBuffer; }
    [[nodiscard]] LineBuffer& get_line_buffer() { return *m_lineBuffer; }

    [[nodiscard]] BoundingSphere calc_bounding_sphere(const linal::float3& center) const;
    [[nodiscard]] linal::float3 calc_scene_centroid() const;

  private:
    [[nodiscard]] std::size_t
    calc_growth_factor(std::size_t currentCapacity, std::size_t freeCapacity, std::size_t requestedCount) const;
};

} // namespace geoqik

#endif // GEOQIK_SOURCE_SCENE_HPP
