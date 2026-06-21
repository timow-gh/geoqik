#ifndef GEOQIK_SOURCE_RENDERING_GEOQIKSCENERENDERER_HPP
#define GEOQIK_SOURCE_RENDERING_GEOQIKSCENERENDERER_HPP

#include "Scene.hpp"
#include <OpenGL/LineType.hpp>

namespace renderer
{
class Renderer;
}

namespace geoqik
{

class GeoQikSceneRenderer
{
  renderer::Renderer& m_renderer;
  opengl::LineType m_lineType{opengl::LineType::lines()};

public:
  explicit GeoQikSceneRenderer(renderer::Renderer& renderer)
      : m_renderer(renderer)
  {
  }

  GeoQikSceneRenderer(const GeoQikSceneRenderer&) = delete;
  GeoQikSceneRenderer& operator=(const GeoQikSceneRenderer&) = delete;
  GeoQikSceneRenderer(GeoQikSceneRenderer&&) = delete;
  GeoQikSceneRenderer& operator=(GeoQikSceneRenderer&&) = delete;

  [[nodiscard]] bool sync_scene(Scene& scene);

  void recreate_point_drawables(const Scene& scene);
  void recreate_line_drawables(const Scene& scene);
  void recreate_mesh_drawables(const Scene& scene);
  void clear_drawables();
};

} // namespace geoqik

#endif // GEOQIK_SOURCE_RENDERING_GEOQIKSCENERENDERER_HPP
