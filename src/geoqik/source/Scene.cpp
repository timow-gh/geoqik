#include "Scene.hpp"

#include "GeometryHelpers.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace geoqik {

namespace {

void translate_vertices(std::vector<float>& vertices, float dx, float dy, float dz) {
    for (std::size_t i = 0; i + 2 < vertices.size(); i += 3) {
        vertices[i] += dx;
        vertices[i + 1] += dy;
        vertices[i + 2] += dz;
    }
}

void rotate_vertices(std::vector<float>& vertices,
                     float centerX,
                     float centerY,
                     float centerZ,
                     float axisX,
                     float axisY,
                     float axisZ,
                     float angle) {
    const linal::float3 center{centerX, centerY, centerZ};
    linal::float3 axis{axisX, axisY, axisZ};
    axis = axis.normalize();
    linal::float33 rotation;
    linal::rot_axis(rotation, axis, angle);
    for (std::size_t i = 0; i + 2 < vertices.size(); i += 3) {
        const linal::float3 point{vertices[i], vertices[i + 1], vertices[i + 2]};
        const auto rotated = rotation * (point - center) + center;
        vertices[i] = rotated[0];
        vertices[i + 1] = rotated[1];
        vertices[i + 2] = rotated[2];
    }
}

void scale_vertices(std::vector<float>& vertices, float cx, float cy, float cz, float sx, float sy, float sz) {
    for (std::size_t i = 0; i + 2 < vertices.size(); i += 3) {
        vertices[i] = cx + (vertices[i] - cx) * sx;
        vertices[i + 1] = cy + (vertices[i + 1] - cy) * sy;
        vertices[i + 2] = cz + (vertices[i + 2] - cz) * sz;
    }
}

void recolor(std::vector<float>& colors, std::size_t vertexCount, const std::array<float, 4>& rgba) {
    colors.resize(vertexCount * ColorChannelCount);
    for (std::size_t i = 0; i < vertexCount; ++i) {
        std::copy(rgba.begin(), rgba.end(), colors.begin() + static_cast<std::ptrdiff_t>(i * ColorChannelCount));
    }
}

} // namespace

Scene Scene::create(const GeoQikSettings& geoqikSettings) {
    Scene scene;
    scene.m_pointBuffer = PointBuffer::create(geoqikSettings);
    scene.m_lineBuffer = LineBuffer::create(geoqikSettings);
    scene.m_meshBuffer = MeshBuffer::create(geoqikSettings);
    scene.m_geomBufferGrowthFactor = std::max<std::size_t>(2, geoqikSettings.capacityGrowthFactor);
    scene.m_pointSize = geoqikSettings.defaultPointSize;
    scene.m_lineWidth = geoqikSettings.defaultLineWidth;
    return scene;
}

bool Scene::ensure_point_capacity(std::size_t pointCount) {
    if (m_pointBuffer->has_space_for_points(pointCount)) {
        return false;
    }

    const std::size_t growthFactor =
        calc_growth_factor(m_pointBuffer->get_point_capacity(), m_pointBuffer->get_free_point_capacity(), pointCount);
    m_pointBuffer = PointBuffer::create_from(*m_pointBuffer, growthFactor);
    return true;
}

bool Scene::ensure_line_capacity(std::size_t lineCount) {
    if (m_lineBuffer->has_space_for_lines(lineCount)) {
        return false;
    }

    const std::size_t growthFactor =
        calc_growth_factor(m_lineBuffer->get_line_capacity(), m_lineBuffer->get_free_line_capacity(), lineCount);
    m_lineBuffer = LineBuffer::create_from(*m_lineBuffer, growthFactor);
    return true;
}

void Scene::add_point(float x, float y, float z, const core::UUID* handle) {
    m_pointBuffer->add_point(x, y, z, handle);
}

void Scene::add_point(float x, float y, float z, float r, float g, float b, float a, const core::UUID* handle) {
    m_pointBuffer->add_point(x, y, z, r, g, b, a, handle);
}

void Scene::add_points(std::span<const float> points, std::span<const float> colors, const core::UUID* handle) {
    m_pointBuffer->add_points(points, colors, handle);
}

bool Scene::update_point(core::UUID handle, float x, float y, float z, std::span<const float> colors) {
    return m_pointBuffer->update_point(handle, x, y, z, colors);
}

bool Scene::update_points(core::UUID handle, std::span<const float> points, std::span<const float> colors) {
    return m_pointBuffer->update_points(handle, points, colors);
}

void Scene::remove_point(core::UUID handle) {
    if (m_styledPoints.erase(handle) > 0) {
        m_styledDirty = true;
        return;
    }
    m_pointBuffer->remove_point(handle);
}

void Scene::add_styled_points(core::UUID handle, StyledPointData data) {
    m_styledPoints.insert_or_assign(handle, std::move(data));
    m_styledDirty = true;
}

bool Scene::update_styled_points(core::UUID handle,
                                 std::span<const float> points,
                                 std::span<const float> colors,
                                 std::span<const float> radii,
                                 std::uint8_t sizeSpace,
                                 bool restyle) {
    auto it = m_styledPoints.find(handle);
    if (it == m_styledPoints.end() || points.size() % 3 != 0 || points.size() != it->second.points.size()) {
        return false;
    }
    const std::size_t pointCount = points.size() / 3;
    if ((!colors.empty() && colors.size() != pointCount * ColorChannelCount) ||
        (restyle && !radii.empty() && radii.size() != 1 && radii.size() != pointCount)) {
        return false;
    }
    if (!restyle && it->second.radii.size() > 1 && it->second.radii.size() != pointCount) {
        return false;
    }
    it->second.points.assign(points.begin(), points.end());
    if (!colors.empty()) {
        it->second.colors.assign(colors.begin(), colors.end());
    }
    if (restyle) {
        it->second.radii.assign(radii.begin(), radii.end());
        it->second.sizeSpace = sizeSpace;
    }
    m_styledDirty = true;
    return true;
}

void Scene::add_line(float x1, float y1, float z1, float x2, float y2, float z2, const core::UUID* handle) {
    m_lineBuffer->add_line(x1, y1, z1, x2, y2, z2, handle);
}

void Scene::add_line(float x1,
                     float y1,
                     float z1,
                     float x2,
                     float y2,
                     float z2,
                     float r,
                     float g,
                     float b,
                     float a,
                     const core::UUID* handle) {
    m_lineBuffer->add_line(x1, y1, z1, x2, y2, z2, r, g, b, a, handle);
}

void Scene::add_lines(std::span<const float> lines, std::span<const float> colors, const core::UUID* handle) {
    m_lineBuffer->add_lines(lines, colors, handle);
}

bool Scene::update_line(core::UUID handle,
                        float x1,
                        float y1,
                        float z1,
                        float x2,
                        float y2,
                        float z2,
                        std::span<const float> colors) {
    return m_lineBuffer->update_line(handle, x1, y1, z1, x2, y2, z2, colors);
}

bool Scene::update_lines(core::UUID handle, std::span<const float> lines, std::span<const float> colors) {
    return m_lineBuffer->update_lines(handle, lines, colors);
}

void Scene::remove_line(core::UUID handle) {
    if (m_styledLines.erase(handle) > 0) {
        m_styledDirty = true;
        return;
    }
    m_lineBuffer->remove_line(handle);
}

void Scene::add_styled_line(core::UUID handle, StyledLineData data) {
    m_styledLines.insert_or_assign(handle, std::move(data));
    m_styledDirty = true;
}

bool Scene::update_styled_line(core::UUID handle,
                               std::span<const float> vertices,
                               std::span<const float> colors,
                               const StrokeStyleData& style,
                               std::uint8_t lineType,
                               std::span<const std::uint8_t> perVertexDashFlags,
                               bool restyle) {
    auto it = m_styledLines.find(handle);
    if (it == m_styledLines.end() || vertices.size() % 3 != 0 || vertices.size() != it->second.vertices.size()) {
        return false;
    }
    const std::size_t vertexCount = vertices.size() / 3;
    if ((!colors.empty() && colors.size() != vertexCount * ColorChannelCount) ||
        (restyle && !perVertexDashFlags.empty() && perVertexDashFlags.size() != vertexCount)) {
        return false;
    }
    if (!restyle && !it->second.perVertexDashFlags.empty() && it->second.perVertexDashFlags.size() != vertexCount) {
        return false;
    }
    it->second.vertices.assign(vertices.begin(), vertices.end());
    if (!colors.empty()) {
        it->second.colors.assign(colors.begin(), colors.end());
    }
    if (restyle) {
        it->second.style = style;
        it->second.lineType = lineType;
        it->second.perVertexDashFlags.assign(perVertexDashFlags.begin(), perVertexDashFlags.end());
    }
    m_styledDirty = true;
    return true;
}

void Scene::add_mesh(std::span<const float> vertices,
                     std::span<const float> normals,
                     std::span<const float> colors,
                     std::span<const std::uint32_t> triangleIndices,
                     const core::UUID* handle) {
    m_meshBuffer->add_mesh(vertices, normals, colors, triangleIndices, handle);
}

void Scene::remove_mesh(core::UUID handle) {
    m_meshBuffer->remove_mesh(handle);
}

bool Scene::update_mesh(core::UUID handle,
                        std::span<const float> vertices,
                        std::span<const float> normals,
                        std::span<const float> colors) {
    return m_meshBuffer->update_mesh(handle, vertices, normals, colors);
}

void Scene::set_mesh_overlay_opts(core::UUID handle, bool showSegments, bool showVertices) {
    m_meshBuffer->set_mesh_segments_visible(handle, showSegments);
    m_meshBuffer->set_mesh_vertices_visible(handle, showVertices);
}

void Scene::set_mesh_rendering_opts(core::UUID handle, PerMeshRenderingOpts opts) {
    m_meshBuffer->set_mesh_rendering_opts(handle, std::move(opts));
}

void Scene::translate_geometry(core::UUID handle, float dx, float dy, float dz) {
    if (auto it = m_styledPoints.find(handle); it != m_styledPoints.end()) {
        translate_vertices(it->second.points, dx, dy, dz);
        m_styledDirty = true;
        return;
    }
    if (auto it = m_styledLines.find(handle); it != m_styledLines.end()) {
        translate_vertices(it->second.vertices, dx, dy, dz);
        m_styledDirty = true;
        return;
    }
    m_pointBuffer->translate_geometry(handle, dx, dy, dz);
    m_lineBuffer->translate_geometry(handle, dx, dy, dz);
    m_meshBuffer->translate_geometry(handle, dx, dy, dz);
}

void Scene::rotate_geometry(core::UUID handle,
                            float centerX,
                            float centerY,
                            float centerZ,
                            float axisX,
                            float axisY,
                            float axisZ,
                            float angle) {
    if (auto it = m_styledPoints.find(handle); it != m_styledPoints.end()) {
        rotate_vertices(it->second.points, centerX, centerY, centerZ, axisX, axisY, axisZ, angle);
        m_styledDirty = true;
        return;
    }
    if (auto it = m_styledLines.find(handle); it != m_styledLines.end()) {
        rotate_vertices(it->second.vertices, centerX, centerY, centerZ, axisX, axisY, axisZ, angle);
        m_styledDirty = true;
        return;
    }
    m_pointBuffer->rotate_geometry(handle, centerX, centerY, centerZ, axisX, axisY, axisZ, angle);
    m_lineBuffer->rotate_geometry(handle, centerX, centerY, centerZ, axisX, axisY, axisZ, angle);
    m_meshBuffer->rotate_geometry(handle, centerX, centerY, centerZ, axisX, axisY, axisZ, angle);
}

void Scene::scale_geometry(core::UUID handle, float cx, float cy, float cz, float sx, float sy, float sz) {
    if (auto it = m_styledPoints.find(handle); it != m_styledPoints.end()) {
        scale_vertices(it->second.points, cx, cy, cz, sx, sy, sz);
        m_styledDirty = true;
        return;
    }
    if (auto it = m_styledLines.find(handle); it != m_styledLines.end()) {
        scale_vertices(it->second.vertices, cx, cy, cz, sx, sy, sz);
        m_styledDirty = true;
        return;
    }
    m_pointBuffer->scale_geometry(handle, cx, cy, cz, sx, sy, sz);
    m_lineBuffer->scale_geometry(handle, cx, cy, cz, sx, sy, sz);
    m_meshBuffer->scale_geometry(handle, cx, cy, cz, sx, sy, sz);
}

void Scene::set_geometry_color(core::UUID handle, float r, float g, float b, float a) {
    const std::array<float, 4> rgba{r, g, b, a};
    if (auto it = m_styledPoints.find(handle); it != m_styledPoints.end()) {
        recolor(it->second.colors, it->second.points.size() / 3, rgba);
        m_styledDirty = true;
        return;
    }
    if (auto it = m_styledLines.find(handle); it != m_styledLines.end()) {
        recolor(it->second.colors, it->second.vertices.size() / 3, rgba);
        m_styledDirty = true;
        return;
    }
    m_pointBuffer->set_geometry_color(handle, rgba);
    m_lineBuffer->set_geometry_color(handle, rgba);
    m_meshBuffer->set_geometry_color(handle, rgba);
}

void Scene::clear() {
    m_pointBuffer->clear();
    m_lineBuffer->clear();
    m_meshBuffer->clear();
    m_styledPoints.clear();
    m_styledLines.clear();
    m_styledDirty = true;
}

SceneSnapshot Scene::create_snapshot() const {
    SceneSnapshot snapshot;
    snapshot.pointBuffer = m_pointBuffer->create_snapshot();
    snapshot.lineBuffer = m_lineBuffer->create_snapshot();
    snapshot.meshBuffer = m_meshBuffer->create_snapshot();
    snapshot.pointSize = m_pointSize;
    snapshot.lineWidth = m_lineWidth;
    snapshot.styledPoints = m_styledPoints;
    snapshot.styledLines = m_styledLines;
    return snapshot;
}

void Scene::restore_snapshot(const SceneSnapshot& snapshot) {
    m_pointBuffer->restore_snapshot(snapshot.pointBuffer);
    m_lineBuffer->restore_snapshot(snapshot.lineBuffer);
    m_meshBuffer->restore_snapshot(snapshot.meshBuffer);
    m_pointSize = snapshot.pointSize;
    m_lineWidth = snapshot.lineWidth;
    m_styledPoints = snapshot.styledPoints;
    m_styledLines = snapshot.styledLines;
    m_styledDirty = true;
}

std::optional<PointBufferGeometry> Scene::get_point_geometry(core::UUID handle) const {
    if (auto it = m_styledPoints.find(handle); it != m_styledPoints.end()) {
        return PointBufferGeometry{it->second.points, it->second.colors};
    }
    return m_pointBuffer->get_geometry(handle);
}

std::optional<LineBufferGeometry> Scene::get_line_geometry(core::UUID handle) const {
    if (auto it = m_styledLines.find(handle); it != m_styledLines.end()) {
        return LineBufferGeometry{it->second.vertices, it->second.colors};
    }
    return m_lineBuffer->get_geometry(handle);
}

bool Scene::is_styled(core::UUID handle) const {
    return m_styledPoints.contains(handle) || m_styledLines.contains(handle);
}

Color Scene::get_default_point_color() const {
    return m_pointBuffer->get_default_point_color();
}

void Scene::set_default_point_color(float r, float g, float b, float a) {
    m_pointBuffer->set_default_point_color(r, g, b, a);
}

Color Scene::get_default_line_color() const {
    return m_lineBuffer->get_default_color();
}

void Scene::set_default_line_color(float r, float g, float b, float a) {
    m_lineBuffer->set_default_color(r, g, b, a);
}

Color Scene::get_default_mesh_color() const {
    return m_meshBuffer->get_default_color();
}

void Scene::set_default_mesh_color(float r, float g, float b, float a) {
    m_meshBuffer->set_default_color(r, g, b, a);
}

BoundingSphere Scene::calc_bounding_sphere(const linal::float3& center) const {
    float maxRadiusSq = 0.0F;

    std::span<const float> points = m_pointBuffer->get_points();
    if (!points.empty()) {
        calc_max_radius_squared(points, center, maxRadiusSq);
    }

    std::span<const float> lines = m_lineBuffer->get_lines();
    if (!lines.empty()) {
        calc_max_radius_squared(lines, center, maxRadiusSq);
    }

    std::span<const float> meshVertices = m_meshBuffer->get_vertices();
    if (!meshVertices.empty()) {
        calc_max_radius_squared(meshVertices, center, maxRadiusSq);
    }
    for (const auto& [uuid, data]: m_styledPoints) {
        (void)uuid;
        calc_max_radius_squared(data.points, center, maxRadiusSq);
    }
    for (const auto& [uuid, data]: m_styledLines) {
        (void)uuid;
        calc_max_radius_squared(data.vertices, center, maxRadiusSq);
    }

    return BoundingSphere{center, std::sqrt(maxRadiusSq)};
}

linal::float3 Scene::calc_scene_centroid() const {
    linal::float3 sum{0.0f, 0.0f, 0.0f};
    std::size_t count = 0;

    auto accumulate = [&](std::span<const float> coords) {
        for (std::size_t i = 0; i + 2 < coords.size(); i += 3) {
            sum[0] += coords[i];
            sum[1] += coords[i + 1];
            sum[2] += coords[i + 2];
            ++count;
        }
    };

    accumulate(m_pointBuffer->get_points());
    accumulate(m_lineBuffer->get_lines());
    accumulate(m_meshBuffer->get_vertices());
    for (const auto& [uuid, data]: m_styledPoints) {
        (void)uuid;
        accumulate(data.points);
    }
    for (const auto& [uuid, data]: m_styledLines) {
        (void)uuid;
        accumulate(data.vertices);
    }

    if (count == 0) {
        return linal::float3{0.0f, 0.0f, 0.0f};
    }

    const float inv = 1.0f / static_cast<float>(count);
    return linal::float3{sum[0] * inv, sum[1] * inv, sum[2] * inv};
}

std::size_t
Scene::calc_growth_factor(std::size_t currentCapacity, std::size_t freeCapacity, std::size_t requestedCount) const {
    if (currentCapacity == 0) {
        return m_geomBufferGrowthFactor;
    }

    const std::size_t usedCapacity = currentCapacity - freeCapacity;
    const std::size_t requiredCapacity = usedCapacity + requestedCount;
    const std::size_t targetCapacity = std::max(m_geomBufferGrowthFactor * currentCapacity, requiredCapacity);
    return (targetCapacity + currentCapacity - 1) / currentCapacity;
}

} // namespace geoqik
