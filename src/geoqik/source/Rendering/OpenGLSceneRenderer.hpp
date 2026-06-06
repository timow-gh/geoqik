#ifndef GEOQIK_SOURCE_RENDERING_OPENGLSCENERENDERER_HPP
#define GEOQIK_SOURCE_RENDERING_OPENGLSCENERENDERER_HPP

#include "Color.hpp"
#include "Scene.hpp"
#include "Viewport.hpp"
#include <OpenGL/Drawable/DrawablesManager.hpp>
#include <OpenGL/LineType.hpp>
#include <OpenGL/Programs/ProgramManager.hpp>
#include <linal/hmat.hpp>
#include <linal/vec.hpp>

namespace geoqik
{

class OpenGLSceneRenderer
{
  opengl::ProgramManager m_programManager;
  opengl::DrawablesManager m_drawablesManager;
  opengl::LineType m_lineType{opengl::LineType::lines()};

public:
  OpenGLSceneRenderer();
  OpenGLSceneRenderer(const OpenGLSceneRenderer&) = delete;
  OpenGLSceneRenderer& operator=(const OpenGLSceneRenderer&) = delete;
  OpenGLSceneRenderer(OpenGLSceneRenderer&&) = delete;
  OpenGLSceneRenderer& operator=(OpenGLSceneRenderer&&) = delete;
  ~OpenGLSceneRenderer();

  void compile_programs();
  void reset_programs() noexcept;

  static void begin_frame(const Color& backgroundColor, const Viewport& viewport);

  void recreate_point_drawables(const Scene& scene);
  void recreate_line_drawables(const Scene& scene);
  [[nodiscard]] bool sync_scene(Scene& scene);

  void draw(const linal::hmatf& mvp, const linal::double3& viewPosition);
  void clear_drawables();

private:
  void create_point_drawable(const Scene& scene);
  void create_line_drawable(const Scene& scene);
};

} // namespace geoqik

#endif // GEOQIK_SOURCE_RENDERING_OPENGLSCENERENDERER_HPP
