#ifndef GEOQIK_SOURCE_RENDERING_GEOQIKSCENERENDERER_HPP
#define GEOQIK_SOURCE_RENDERING_GEOQIKSCENERENDERER_HPP

#include "Scene.hpp"
#include <OpenGL/LineType.hpp>
#include <plinth/Renderer.hpp>
#include <unordered_map>

namespace geoqik {

class GeoQikSceneRenderer {
    renderer::Renderer& m_renderer;
    opengl::LineType m_lineType{opengl::LineType::lines()};

    struct MeshDrawableBundle {
        renderer::DrawableHandle surface;
        renderer::DrawableHandle segments;
        renderer::DrawableHandle vertices;
    };

    std::unordered_map<core::UUID, MeshDrawableBundle> m_meshBundles;

  public:
    explicit GeoQikSceneRenderer(renderer::Renderer& renderer)
        : m_renderer(renderer) {}

    GeoQikSceneRenderer(const GeoQikSceneRenderer&) = delete;
    GeoQikSceneRenderer& operator=(const GeoQikSceneRenderer&) = delete;
    GeoQikSceneRenderer(GeoQikSceneRenderer&&) = delete;
    GeoQikSceneRenderer& operator=(GeoQikSceneRenderer&&) = delete;

    [[nodiscard]]
    bool sync_scene(Scene& scene);

    void recreate_point_drawables(const Scene& scene);
    void recreate_line_drawables(const Scene& scene);
    void recreate_mesh_drawables(const Scene& scene);
    void clear_drawables();

  private:
    bool sync_points(Scene& scene);
    bool sync_lines(Scene& scene);
    bool sync_meshes(MeshBuffer& meshBuffer);
    bool sync_mesh_changes(MeshBuffer& meshBuffer);
    bool sync_overlay_drawables(MeshBuffer& meshBuffer);
    void create_surface_bundle(const core::UUID& uuid, const MeshBuffer& meshBuffer);
    void remove_bundle(const core::UUID& uuid);
};

} // namespace geoqik

#endif // GEOQIK_SOURCE_RENDERING_GEOQIKSCENERENDERER_HPP
