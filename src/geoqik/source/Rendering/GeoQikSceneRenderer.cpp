#include "Rendering/GeoQikSceneRenderer.hpp"

#include "GeometryBuffers/MeshBuffer.hpp"
#include "Rendering/StyleTranslation.hpp"

#include <plinth/BufferAccessPattern.hpp>
#include <plinth/MeshCullFaceMode.hpp>
#include <plinth/Renderer.hpp>
#include <plinth/SphereStyle.hpp>
#include <plinth/StrokeStyle.hpp>

#include <array>
#include <vector>

namespace geoqik {

namespace {

[[nodiscard]] std::vector<float> radii_for(std::span<const float> centers, float radius) {
    return std::vector<float>(centers.size() / 3, radius);
}

[[nodiscard]] std::vector<std::uint32_t> sequential_indices(std::size_t vertexCount) {
    std::vector<std::uint32_t> indices(vertexCount);
    for (std::size_t i = 0; i < vertexCount; ++i) {
        indices[i] = static_cast<std::uint32_t>(i);
    }
    return indices;
}

struct LineDrawableInputs {
    std::vector<float> vertices;
    std::vector<float> colors;
    std::vector<std::uint8_t> dashFlags;
    std::vector<std::uint32_t> indices;
};

[[nodiscard]] LineDrawableInputs build_line_drawable_inputs(const StyledLineData& data) {
    LineDrawableInputs result;
    if (data.lineType == GEOQIK_LINE_TYPE_LINES || data.vertices.size() < 6) {
        result.vertices = data.vertices;
        result.colors = data.colors;
        result.dashFlags = data.perVertexDashFlags;
    } else {
        const std::size_t rawVertexCount = data.vertices.size() / 3;
        const auto append_vertex = [&](std::size_t vertexIndex) {
            const auto vertexBegin = data.vertices.begin() + static_cast<std::ptrdiff_t>(vertexIndex * 3);
            result.vertices.insert(result.vertices.end(), vertexBegin, vertexBegin + 3);
            if (data.colors.size() >= (vertexIndex + 1) * ColorChannelCount) {
                const auto colorBegin =
                    data.colors.begin() + static_cast<std::ptrdiff_t>(vertexIndex * ColorChannelCount);
                result.colors.insert(result.colors.end(), colorBegin, colorBegin + ColorChannelCount);
            }
            if (!data.perVertexDashFlags.empty()) {
                result.dashFlags.push_back(data.perVertexDashFlags[vertexIndex]);
            }
        };
        append_vertex(0);
        for (std::size_t vertexIndex = 1; vertexIndex < rawVertexCount; vertexIndex += 2) {
            append_vertex(vertexIndex);
        }
        if (data.lineType == GEOQIK_LINE_TYPE_LINE_LOOP && result.vertices.size() >= 6) {
            const auto last = result.vertices.size() - 3;
            if (result.vertices[0] == result.vertices[last] && result.vertices[1] == result.vertices[last + 1] &&
                result.vertices[2] == result.vertices[last + 2]) {
                result.vertices.resize(last);
                result.colors.resize(result.colors.size() - ColorChannelCount);
                if (!result.dashFlags.empty()) {
                    result.dashFlags.pop_back();
                }
            }
        }
    }
    result.indices = sequential_indices(result.vertices.size() / 3);
    return result;
}

} // namespace

bool GeoQikSceneRenderer::sync_points(Scene& scene) {
    auto& pointBuffer = scene.get_point_buffer();
    if (pointBuffer.points_have_changed() || (!m_mergedPointDrawable.is_valid() && !pointBuffer.get_points().empty())) {
        if (m_mergedPointDrawable.is_valid()) {
            m_renderer.remove_drawable(m_mergedPointDrawable);
            m_mergedPointDrawable = {};
        }
        if (!pointBuffer.get_points().empty()) {
            auto radii = radii_for(pointBuffer.get_points(), scene.get_point_size());
            m_mergedPointDrawable =
                m_renderer.add_sphere_point_drawable(pointBuffer.get_points(),
                                                     radii,
                                                     pointBuffer.get_point_colors(),
                                                     renderer::SphereStyle{renderer::SphereSizeSpace::Screen},
                                                     renderer::BufferAccessPattern::Static);
        }
        pointBuffer.reset_points_have_changed();
        return true;
    }
    return false;
}

bool GeoQikSceneRenderer::sync_lines(Scene& scene) {
    auto& lineBuffer = scene.get_line_buffer();
    if (lineBuffer.lines_have_changed() || (!m_mergedLineDrawable.is_valid() && !lineBuffer.get_lines().empty())) {
        if (m_mergedLineDrawable.is_valid()) {
            m_renderer.remove_drawable(m_mergedLineDrawable);
            m_mergedLineDrawable = {};
        }
        if (!lineBuffer.get_lines().empty()) {
            renderer::StrokeStyle style;
            style.lineWidth = scene.get_line_width();
            style.dashSpace = renderer::DashSpace::World;
            m_mergedLineDrawable = m_renderer.add_line_drawable(lineBuffer.get_lines(),
                                                                 lineBuffer.get_line_indices(),
                                                                 lineBuffer.get_line_colors(),
                                                                 m_lineType,
                                                                 style,
                                                                 0.0F,
                                                                 renderer::BufferAccessPattern::Static);
        }
        lineBuffer.reset_lines_have_changed();
        return true;
    }
    return false;
}

bool GeoQikSceneRenderer::sync_styled(Scene& scene) {
    if (!scene.styled_dirty()) {
        return false;
    }
    for (const auto& [uuid, handle]: m_styledPointBundles) {
        (void)uuid;
        m_renderer.remove_drawable(handle);
    }
    for (const auto& [uuid, handle]: m_styledLineBundles) {
        (void)uuid;
        m_renderer.remove_drawable(handle);
    }
    m_styledPointBundles.clear();
    m_styledLineBundles.clear();
    for (const auto& [uuid, data]: scene.get_styled_points()) {
        create_styled_point_drawable(uuid, data, scene.get_point_size());
    }
    for (const auto& [uuid, data]: scene.get_styled_lines()) {
        create_styled_line_drawable(uuid, data, scene.get_line_width());
    }
    scene.reset_styled_dirty();
    return true;
}

bool GeoQikSceneRenderer::sync_mesh_changes(MeshBuffer& meshBuffer) {
    bool updateOccurred = false;
    for (const core::UUID& uuid: meshBuffer.get_removed_meshes()) {
        remove_bundle(uuid);
    }
    if (!meshBuffer.get_removed_meshes().empty()) {
        updateOccurred = true;
    }

    for (const core::UUID& uuid: meshBuffer.get_updated_meshes()) {
        remove_bundle(uuid);
        create_surface_bundle(uuid, meshBuffer);
    }
    if (!meshBuffer.get_updated_meshes().empty()) {
        updateOccurred = true;
    }

    for (const core::UUID& uuid: meshBuffer.get_added_meshes()) {
        create_surface_bundle(uuid, meshBuffer);
    }
    if (!meshBuffer.get_added_meshes().empty()) {
        updateOccurred = true;
    }
    return updateOccurred;
}

bool GeoQikSceneRenderer::sync_meshes(MeshBuffer& meshBuffer) {
    if (meshBuffer.is_full_rebuild_needed()) {
        for (auto& [uuid, bundle]: m_meshBundles) {
            if (bundle.surface.is_valid()) {
                m_renderer.remove_drawable(bundle.surface);
            }
            if (bundle.segments.is_valid()) {
                m_renderer.remove_drawable(bundle.segments);
            }
            if (bundle.vertices.is_valid()) {
                m_renderer.remove_drawable(bundle.vertices);
            }
        }
        m_meshBundles.clear();
        for (const core::UUID& uuid: meshBuffer.get_all_mesh_uuids()) {
            create_surface_bundle(uuid, meshBuffer);
        }
        return true;
    }
    return sync_mesh_changes(meshBuffer);
}

bool GeoQikSceneRenderer::sync_overlay_drawables(MeshBuffer& meshBuffer) {
    bool updateOccurred = false;
    for (auto& [uuid, bundle]: m_meshBundles) {
        if (!meshBuffer.has_mesh_overlay_data(uuid)) {
            continue;
        }
        const auto& overlay = meshBuffer.get_mesh_overlay_data(uuid);

        const bool wantSegments =
            overlay.showSegments && !overlay.segmentPositions.empty() && !overlay.segmentIndices.empty();
        if (wantSegments && !bundle.segments.is_valid()) {
            const std::vector<float> colorVec{overlay.segmentColor[0],
                                              overlay.segmentColor[1],
                                              overlay.segmentColor[2],
                                              overlay.segmentColor[3]};
            bundle.segments = m_renderer.add_line_drawable(std::span<const float>(overlay.segmentPositions),
                                                           std::span<const std::uint32_t>(overlay.segmentIndices),
                                                           std::span<const float>(colorVec),
                                                           renderer::LineType::lines(),
                                                           renderer::StrokeStyle{overlay.segmentLineWidth});
            updateOccurred = true;
        } else if (!wantSegments && bundle.segments.is_valid()) {
            m_renderer.remove_drawable(bundle.segments);
            bundle.segments = {};
            updateOccurred = true;
        }

        const bool wantVertices = overlay.showVertices && !overlay.segmentPositions.empty();
        if (wantVertices && !bundle.vertices.is_valid()) {
            const std::array<float, 4> colorArr{overlay.vertexColor[0],
                                                overlay.vertexColor[1],
                                                overlay.vertexColor[2],
                                                overlay.vertexColor[3]};
            auto radii = radii_for(overlay.segmentPositions, overlay.vertexPointSize);
            bundle.vertices =
                m_renderer.add_sphere_point_drawable(std::span<const float>(overlay.segmentPositions), radii, colorArr);
            updateOccurred = true;
        } else if (!wantVertices && bundle.vertices.is_valid()) {
            m_renderer.remove_drawable(bundle.vertices);
            bundle.vertices = {};
            updateOccurred = true;
        }
    }
    return updateOccurred;
}

bool GeoQikSceneRenderer::sync_scene(Scene& scene) {
    bool updateOccurred = false;
    updateOccurred |= sync_points(scene);
    updateOccurred |= sync_lines(scene);
    updateOccurred |= sync_styled(scene);

    auto& meshBuffer = scene.get_mesh_buffer();
    updateOccurred |= sync_meshes(meshBuffer);
    meshBuffer.clear_change_tracking();

    // Toggle overlay drawables on/off based on current visibility flags.
    // This runs every sync to pick up flag-only changes that don't trigger m_updatedMeshes.
    updateOccurred |= sync_overlay_drawables(meshBuffer);

    return updateOccurred;
}

void GeoQikSceneRenderer::recreate_point_drawables(const Scene& scene) {
    if (m_mergedPointDrawable.is_valid()) {
        m_renderer.remove_drawable(m_mergedPointDrawable);
        m_mergedPointDrawable = {};
    }
    for (const auto& [uuid, handle]: m_styledPointBundles) {
        (void)uuid;
        m_renderer.remove_drawable(handle);
    }
    m_styledPointBundles.clear();

    const auto& pointBuffer = scene.get_point_buffer();
    if (!pointBuffer.get_points().empty()) {
        auto radii = radii_for(pointBuffer.get_points(), scene.get_point_size());
        m_mergedPointDrawable =
            m_renderer.add_sphere_point_drawable(pointBuffer.get_points(),
                                                 radii,
                                                 pointBuffer.get_point_colors(),
                                                 renderer::SphereStyle{renderer::SphereSizeSpace::Screen},
                                                 renderer::BufferAccessPattern::Static);
    }
    for (const auto& [uuid, data]: scene.get_styled_points()) {
        create_styled_point_drawable(uuid, data, scene.get_point_size());
    }
}

void GeoQikSceneRenderer::recreate_line_drawables(const Scene& scene) {
    if (m_mergedLineDrawable.is_valid()) {
        m_renderer.remove_drawable(m_mergedLineDrawable);
        m_mergedLineDrawable = {};
    }
    for (const auto& [uuid, handle]: m_styledLineBundles) {
        (void)uuid;
        m_renderer.remove_drawable(handle);
    }
    m_styledLineBundles.clear();

    const auto& lineBuffer = scene.get_line_buffer();
    if (!lineBuffer.get_lines().empty()) {
        renderer::StrokeStyle style;
        style.lineWidth = scene.get_line_width();
        style.dashSpace = renderer::DashSpace::World;
        m_mergedLineDrawable = m_renderer.add_line_drawable(lineBuffer.get_lines(),
                                                            lineBuffer.get_line_indices(),
                                                            lineBuffer.get_line_colors(),
                                                            m_lineType,
                                                            style,
                                                            0.0F,
                                                            renderer::BufferAccessPattern::Static);
    }
    for (const auto& [uuid, data]: scene.get_styled_lines()) {
        create_styled_line_drawable(uuid, data, scene.get_line_width());
    }
}

void GeoQikSceneRenderer::recreate_mesh_drawables(const Scene& scene) {
    for (auto& [uuid, bundle]: m_meshBundles) {
        if (bundle.surface.is_valid()) {
            m_renderer.remove_drawable(bundle.surface);
        }
        if (bundle.segments.is_valid()) {
            m_renderer.remove_drawable(bundle.segments);
        }
        if (bundle.vertices.is_valid()) {
            m_renderer.remove_drawable(bundle.vertices);
        }
    }
    m_meshBundles.clear();

    for (const core::UUID& uuid: scene.get_mesh_buffer().get_all_mesh_uuids()) {
        create_surface_bundle(uuid, scene.get_mesh_buffer());
    }
}

void GeoQikSceneRenderer::clear_drawables() {
    m_renderer.clear_drawables();
    m_mergedPointDrawable = {};
    m_mergedLineDrawable = {};
    m_meshBundles.clear();
    m_styledPointBundles.clear();
    m_styledLineBundles.clear();
}

void GeoQikSceneRenderer::rebuild_all_drawables(const Scene& scene) {
    clear_drawables();
    const auto& pointBuffer = scene.get_point_buffer();
    if (!pointBuffer.get_points().empty()) {
        auto radii = radii_for(pointBuffer.get_points(), scene.get_point_size());
        m_mergedPointDrawable =
            m_renderer.add_sphere_point_drawable(pointBuffer.get_points(),
                                                 radii,
                                                 pointBuffer.get_point_colors(),
                                                 renderer::SphereStyle{renderer::SphereSizeSpace::Screen},
                                                 renderer::BufferAccessPattern::Static);
    }
    const auto& lineBuffer = scene.get_line_buffer();
    if (!lineBuffer.get_lines().empty()) {
        renderer::StrokeStyle style;
        style.lineWidth = scene.get_line_width();
        style.dashSpace = renderer::DashSpace::World;
        m_mergedLineDrawable = m_renderer.add_line_drawable(lineBuffer.get_lines(),
                                                            lineBuffer.get_line_indices(),
                                                            lineBuffer.get_line_colors(),
                                                            m_lineType,
                                                            style,
                                                            0.0F,
                                                            renderer::BufferAccessPattern::Static);
    }
    for (const auto& [uuid, data]: scene.get_styled_points()) {
        create_styled_point_drawable(uuid, data, scene.get_point_size());
    }
    for (const auto& [uuid, data]: scene.get_styled_lines()) {
        create_styled_line_drawable(uuid, data, scene.get_line_width());
    }
    for (const core::UUID& uuid: scene.get_mesh_buffer().get_all_mesh_uuids()) {
        create_surface_bundle(uuid, scene.get_mesh_buffer());
    }
}

void GeoQikSceneRenderer::create_styled_point_drawable(const core::UUID& uuid,
                                                       const StyledPointData& data,
                                                       float fallbackRadius) {
    std::vector<float> expandedRadii;
    std::span<const float> radii = data.radii;
    if (radii.empty()) {
        expandedRadii = radii_for(data.points, fallbackRadius);
        radii = expandedRadii;
    } else if (radii.size() == 1 && data.points.size() / 3 > 1) {
        expandedRadii.assign(data.points.size() / 3, radii.front());
        radii = expandedRadii;
    }
    const auto handle =
        m_renderer.add_sphere_point_drawable(data.points,
                                             radii,
                                             data.colors,
                                             renderer::SphereStyle{to_plinth_size_space(data.sizeSpace)},
                                             renderer::BufferAccessPattern::Static);
    m_styledPointBundles.emplace(uuid, handle);
}

void GeoQikSceneRenderer::create_styled_line_drawable(const core::UUID& uuid,
                                                      const StyledLineData& data,
                                                      float fallbackWidth) {
    renderer::StrokeStyle style;
    style.lineWidth = data.style.lineWidth > 0.0F ? data.style.lineWidth : fallbackWidth;
    style.cap = to_plinth_cap(data.style.cap);
    style.join = to_plinth_join(data.style.join);
    style.miterLimit = data.style.miterLimit > 0.0F ? data.style.miterLimit : 4.0F;
    style.dashPattern = data.style.dashPattern;
    style.dashPhase = data.style.dashPhase;
    style.dashSpace = to_plinth_dash_space(data.style.dashSpace);
    const auto inputs = build_line_drawable_inputs(data);
    const auto handle = m_renderer.add_line_drawable(inputs.vertices,
                                                     inputs.indices,
                                                     inputs.colors,
                                                     to_plinth_line_type(data.lineType),
                                                     style,
                                                     0.0F,
                                                     renderer::BufferAccessPattern::Static,
                                                     inputs.dashFlags);
    m_styledLineBundles.emplace(uuid, handle);
}

void GeoQikSceneRenderer::create_surface_bundle(const core::UUID& uuid, const MeshBuffer& meshBuffer) {
    remove_bundle(uuid);

    MeshDrawableBundle bundle;

    bool surfaceVisible = true;
    if (meshBuffer.has_mesh_rendering_opts(uuid)) {
        const auto& renderOpts = meshBuffer.get_mesh_rendering_opts(uuid);
        surfaceVisible = renderOpts.surfaceVisible;
    }

    if (surfaceVisible) {
        bundle.surface = m_renderer.add_mesh_drawable(meshBuffer.get_mesh_vertices(uuid),
                                                      meshBuffer.get_local_triangle_indices(uuid),
                                                      meshBuffer.get_mesh_normals(uuid),
                                                      meshBuffer.get_mesh_colors(uuid),
                                                      renderer::MeshCullFaceMode::BACK,
                                                      renderer::BufferAccessPattern::Static);

        if (bundle.surface.is_valid() && meshBuffer.has_mesh_rendering_opts(uuid)) {
            const auto& renderOpts = meshBuffer.get_mesh_rendering_opts(uuid);
            renderer::MeshCullFaceMode cullFaceMode = renderer::MeshCullFaceMode::BACK;
            switch (renderOpts.cullMode) {
            case MeshCullMode::front: cullFaceMode = renderer::MeshCullFaceMode::FRONT; break;
            case MeshCullMode::none:  cullFaceMode = renderer::MeshCullFaceMode::NONE; break;
            case MeshCullMode::back:
            default:                  cullFaceMode = renderer::MeshCullFaceMode::BACK; break;
            }
            m_renderer.set_mesh_drawable_cull_mode(bundle.surface, cullFaceMode);
        }
    }

    // Segment overlay
    if (meshBuffer.has_mesh_overlay_data(uuid)) {
        const auto& overlay = meshBuffer.get_mesh_overlay_data(uuid);
        if (overlay.showSegments && !overlay.segmentPositions.empty() && !overlay.segmentIndices.empty()) {
            const std::vector<float> colorVec{overlay.segmentColor[0],
                                              overlay.segmentColor[1],
                                              overlay.segmentColor[2],
                                              overlay.segmentColor[3]};
            bundle.segments = m_renderer.add_line_drawable(std::span<const float>(overlay.segmentPositions),
                                                           std::span<const std::uint32_t>(overlay.segmentIndices),
                                                           std::span<const float>(colorVec),
                                                           renderer::LineType::lines(),
                                                           renderer::StrokeStyle{overlay.segmentLineWidth});
        }

        // Vertex overlay
        if (overlay.showVertices && !overlay.segmentPositions.empty()) {
            const std::array<float, 4> colorArr{overlay.vertexColor[0],
                                                overlay.vertexColor[1],
                                                overlay.vertexColor[2],
                                                overlay.vertexColor[3]};
            auto radii = radii_for(overlay.segmentPositions, overlay.vertexPointSize);
            bundle.vertices =
                m_renderer.add_sphere_point_drawable(std::span<const float>(overlay.segmentPositions), radii, colorArr);
        }
    }

    m_meshBundles.emplace(uuid, bundle);
}

void GeoQikSceneRenderer::remove_bundle(const core::UUID& uuid) {
    auto it = m_meshBundles.find(uuid);
    if (it == m_meshBundles.end()) {
        return;
    }
    auto& bundle = it->second;
    if (bundle.surface.is_valid()) {
        m_renderer.remove_drawable(bundle.surface);
    }
    if (bundle.segments.is_valid()) {
        m_renderer.remove_drawable(bundle.segments);
    }
    if (bundle.vertices.is_valid()) {
        m_renderer.remove_drawable(bundle.vertices);
    }
    m_meshBundles.erase(it);
}

} // namespace geoqik
