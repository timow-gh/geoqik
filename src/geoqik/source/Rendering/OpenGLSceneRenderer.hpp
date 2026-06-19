#ifndef GEOQIK_SOURCE_RENDERING_OPENGLSCENERENDERER_HPP
#define GEOQIK_SOURCE_RENDERING_OPENGLSCENERENDERER_HPP

#include <Renderer/Color.hpp>
#include "Scene.hpp"
#include <Renderer/Viewport.hpp>
#include <OpenGL/Drawable/DrawablesManager.hpp>
#include <OpenGL/LineType.hpp>
#include <linal/hmat.hpp>
#include <linal/vec.hpp>
#include <memory>

namespace geoqik
{

struct MeshRenderParams
{
  linal::hmatf modelMatrix;
  linal::hmatf viewMatrix;
  linal::hmatf projectionMatrix;
  linal::hmatf normalMatrix;
  linal::float3 lightPosition;
  linal::float3 viewPos;
  linal::float3 lightColor;
  linal::float3 fillLightDirection;
  linal::float3 fillLightColor;
  linal::float3 ambientColor;
  float shininess{32.0f};
};

using renderer::Color;
using renderer::Viewport;

class OpenGLSceneRenderer
{
  opengl::DrawablesManager m_drawablesManager;
  opengl::LineType m_lineType{opengl::LineType::lines()};

public:
  OpenGLSceneRenderer(const OpenGLSceneRenderer&) = delete;
  OpenGLSceneRenderer& operator=(const OpenGLSceneRenderer&) = delete;
  OpenGLSceneRenderer(OpenGLSceneRenderer&&) = default;
  OpenGLSceneRenderer& operator=(OpenGLSceneRenderer&&) = default;
  ~OpenGLSceneRenderer();

  static std::unique_ptr<OpenGLSceneRenderer> create();

  static void begin_frame(const Color& backgroundColor, const Viewport& viewport);

  void recreate_point_drawables(const Scene& scene);
  void recreate_line_drawables(const Scene& scene);
  void recreate_mesh_drawables(const Scene& scene);
  [[nodiscard]] bool sync_scene(Scene& scene);

  void draw(const linal::hmatf& mvp, const linal::double3& viewPosition, const MeshRenderParams& meshParams);
  void clear_drawables();

private:
  OpenGLSceneRenderer(opengl::DrawablesManager drawablesManager)
      : m_drawablesManager(std::move(drawablesManager))
  {
  }

  void create_point_drawable(const Scene& scene);
  void create_line_drawable(const Scene& scene);
  void create_mesh_drawable(const Scene& scene);
};

} // namespace geoqik

#endif // GEOQIK_SOURCE_RENDERING_OPENGLSCENERENDERER_HPP
