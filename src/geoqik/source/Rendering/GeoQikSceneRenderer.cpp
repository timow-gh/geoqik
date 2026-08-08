#include "Rendering/GeoQikSceneRenderer.hpp"

#include "GeometryBuffers/MeshBuffer.hpp"

#include <plinth/BufferAccessPattern.hpp>
#include <plinth/MeshCullFaceMode.hpp>
#include <plinth/Renderer.hpp>
#include <plinth/StrokeStyle.hpp>

#include <array>
#include <vector>

namespace geoqik {

namespace {

[[nodiscard]] std::vector<float> radii_for(std::span<const float> centers, float radius) {
    return std::vector<float>(centers.size() / 3, radius);
}

} // namespace

bool GeoQikSceneRenderer::sync_points(Scene& scene) {
    auto& pointBuffer = scene.get_point_buffer();
    if (!m_renderer.has_sphere_point_drawables() && !pointBuffer.get_points().empty()) {
        auto radii = radii_for(pointBuffer.get_points(), scene.get_point_size());
        m_renderer.add_sphere_point_drawable(pointBuffer.get_points(),
                                             radii,
                                             pointBuffer.get_point_colors(),
                                             renderer::BufferAccessPattern::Static);
        return true;
    }
    if (pointBuffer.points_have_changed()) {
        m_renderer.clear_sphere_point_drawables();
        if (!pointBuffer.get_points().empty()) {
            auto radii = radii_for(pointBuffer.get_points(), scene.get_point_size());
            m_renderer.add_sphere_point_drawable(pointBuffer.get_points(),
                                                 radii,
                                                 pointBuffer.get_point_colors(),
                                                 renderer::BufferAccessPattern::Static);
        }
        pointBuffer.reset_points_have_changed();
        return true;
    }
    return false;
}

bool GeoQikSceneRenderer::sync_lines(Scene& scene) {
    auto& lineBuffer = scene.get_line_buffer();
    if (!m_renderer.has_line_drawables() && !lineBuffer.get_lines().empty()) {
        m_renderer.add_line_drawable(lineBuffer.get_lines(),
                                     lineBuffer.get_line_indices(),
                                     lineBuffer.get_line_colors(),
                                     m_lineType,
                                     renderer::StrokeStyle{scene.get_line_width()},
                                     scene.get_point_size(),
                                     renderer::BufferAccessPattern::Static);
        return true;
    }
    if (lineBuffer.lines_have_changed()) {
        m_renderer.update_last_line_drawable(lineBuffer.get_lines(),
                                             lineBuffer.get_line_colors(),
                                             lineBuffer.get_line_indices(),
                                             renderer::BufferAccessPattern::Static);
        lineBuffer.reset_lines_have_changed();
        return true;
    }
    return false;
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
            bundle.vertices = m_renderer.add_sphere_point_drawable(std::span<const float>(overlay.segmentPositions),
                                                                   radii,
                                                                   colorArr);
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

    auto& meshBuffer = scene.get_mesh_buffer();
    updateOccurred |= sync_meshes(meshBuffer);
    meshBuffer.clear_change_tracking();

    // Toggle overlay drawables on/off based on current visibility flags.
    // This runs every sync to pick up flag-only changes that don't trigger m_updatedMeshes.
    updateOccurred |= sync_overlay_drawables(meshBuffer);

    return updateOccurred;
}

void GeoQikSceneRenderer::recreate_point_drawables(const Scene& scene) {
    m_renderer.clear_drawables();
    const auto& pointBuffer = scene.get_point_buffer();
    if (pointBuffer.get_points().empty()) {
        return;
    }
    auto radii = radii_for(pointBuffer.get_points(), scene.get_point_size());
    m_renderer.add_sphere_point_drawable(pointBuffer.get_points(),
                                         radii,
                                         pointBuffer.get_point_colors(),
                                         renderer::BufferAccessPattern::Static);
}

void GeoQikSceneRenderer::recreate_line_drawables(const Scene& scene) {
    m_renderer.clear_drawables();
    const auto& pointBuffer = scene.get_point_buffer();
    if (!pointBuffer.get_points().empty()) {
        auto radii = radii_for(pointBuffer.get_points(), scene.get_point_size());
        m_renderer.add_sphere_point_drawable(pointBuffer.get_points(),
                                             radii,
                                             pointBuffer.get_point_colors(),
                                             renderer::BufferAccessPattern::Static);
    }
    const auto& lineBuffer = scene.get_line_buffer();
    if (!lineBuffer.get_lines().empty()) {
        m_renderer.add_line_drawable(lineBuffer.get_lines(),
                                     lineBuffer.get_line_indices(),
                                     lineBuffer.get_line_colors(),
                                     m_lineType,
                                     renderer::StrokeStyle{scene.get_line_width()},
                                     scene.get_point_size(),
                                     renderer::BufferAccessPattern::Static);
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
            bundle.vertices = m_renderer.add_sphere_point_drawable(std::span<const float>(overlay.segmentPositions),
                                                                   radii,
                                                                   colorArr);
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
