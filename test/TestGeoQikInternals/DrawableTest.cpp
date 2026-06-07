#include <OpenGL/Drawable/LineDrawable.hpp>
#include <OpenGL/Drawable/MeshDrawable.hpp>
#include <OpenGL/Drawable/PointDrawable.hpp>
#include <OpenGL/OpenGL.hpp>
#include <gtest/gtest.h>
#include <GLFW/glfw3.h>
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace
{

void* load_glfw_proc(const char* procName)
{
  // GLAD v1 expects void* while GLFW returns an opaque function pointer.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<void*>(glfwGetProcAddress(procName));
}

class OpenGLDrawableTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_EQ(GLFW_TRUE, glfwInit());
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(64, 64, "GeoQik drawable test", nullptr, nullptr);
    ASSERT_NE(nullptr, m_window);
    glfwMakeContextCurrent(m_window);
    ASSERT_NE(0, gladLoadGLLoader(load_glfw_proc));
  }

  void TearDown() override
  {
    if (m_window != nullptr)
    {
      glfwDestroyWindow(m_window);
      m_window = nullptr;
    }
    glfwTerminate();
  }

  GLFWwindow* m_window{nullptr};
};

linal::hmatf matrix()
{
  linal::hmatf result;
  return result;
}

opengl::PointDrawable make_point_drawable_with_alpha(opengl::PointProgram& program, const std::vector<float>& colors)
{
  const std::vector<float> vertices = {
      0.0F, 0.0F, 0.0F,
      1.0F, 0.0F, 0.0F,
      0.0F, 1.0F, 0.0F,
  };
  const std::vector<std::uint32_t> indices = {0U, 1U, 2U};

  std::optional<opengl::PointDrawable> drawable = opengl::make_point_drawable(program,
                                                                              vertices,
                                                                              3,
                                                                              colors,
                                                                              4,
                                                                              indices,
                                                                              3.0F,
                                                                              opengl::BufferAccessPattern::STATIC_DRAW);
  EXPECT_TRUE(drawable.has_value());
  return std::move(drawable.value());
}

opengl::LineDrawable make_line_drawable_with_alpha(opengl::LineProgram& program, const std::vector<float>& colors, float pointSize)
{
  const std::vector<float> vertices = {
      0.0F, 0.0F, 0.0F,
      1.0F, 0.0F, 0.0F,
      0.0F, 1.0F, 0.0F,
      1.0F, 1.0F, 0.0F,
  };
  const std::vector<std::uint32_t> indices = {0U, 1U, 2U, 3U};

  std::optional<opengl::LineDrawable> drawable = opengl::make_line_drawable(program,
                                                                            vertices,
                                                                            3,
                                                                            indices,
                                                                            colors,
                                                                            4,
                                                                            opengl::LineType::lines(),
                                                                            2.0F,
                                                                            pointSize,
                                                                            opengl::BufferAccessPattern::STATIC_DRAW);
  EXPECT_TRUE(drawable.has_value());
  return std::move(drawable.value());
}

opengl::MeshDrawable make_mesh_drawable_with_alpha(opengl::MeshProgram& program, const std::vector<float>& colors)
{
  const std::vector<float> vertices = {
      0.0F, 0.0F, 0.0F,
      1.0F, 0.0F, 0.0F,
      0.0F, 1.0F, 0.0F,
  };
  const std::vector<float> normals = {
      0.0F, 0.0F, 1.0F,
      0.0F, 0.0F, 1.0F,
      0.0F, 0.0F, 1.0F,
  };
  const std::vector<std::uint32_t> indices = {0U, 1U, 2U};

  std::optional<opengl::MeshDrawable> drawable = opengl::make_mesh_soup(program,
                                                                        vertices,
                                                                        3,
                                                                        normals,
                                                                        colors,
                                                                        4,
                                                                        indices,
                                                                        opengl::BufferAccessPattern::STATIC_DRAW);
  EXPECT_TRUE(drawable.has_value());
  return std::move(drawable.value());
}

} // namespace

TEST_F(OpenGLDrawableTest, PointDrawableUpdatesDrawsAndMoveAssigns)
{
  opengl::PointProgram program = opengl::make_point_program();
  const std::vector<float> mixedColors = {
      1.0F, 0.0F, 0.0F, 1.0F,
      0.0F, 1.0F, 0.0F, 0.5F,
      0.0F, 0.0F, 1.0F, 1.0F,
  };
  opengl::PointDrawable drawable = make_point_drawable_with_alpha(program, mixedColors);
  EXPECT_TRUE(drawable.has_opaque_primitives());
  EXPECT_TRUE(drawable.has_translucent_primitives());

  const std::vector<float> updatedVertices = {
      -1.0F, 0.0F, 0.0F,
      0.0F, 0.0F, 0.0F,
      1.0F, 0.0F, 0.0F,
  };
  const std::vector<float> opaqueColors = {
      1.0F, 0.0F, 0.0F, 1.0F,
      0.0F, 1.0F, 0.0F, 1.0F,
      0.0F, 0.0F, 1.0F, 1.0F,
  };
  const std::vector<std::uint32_t> emptyIndices;
  drawable.update_point_drawable(updatedVertices, opaqueColors, emptyIndices, opengl::BufferAccessPattern::DYNAMIC_DRAW);
  EXPECT_FALSE(drawable.has_opaque_primitives());
  EXPECT_FALSE(drawable.has_translucent_primitives());
  drawable.draw_opaque(matrix());

  const std::vector<std::uint32_t> translucentIndices = {1U};
  drawable.update_point_drawable(updatedVertices, mixedColors, translucentIndices, opengl::BufferAccessPattern::DYNAMIC_DRAW);
  EXPECT_FALSE(drawable.has_opaque_primitives());
  EXPECT_TRUE(drawable.has_translucent_primitives());
  drawable.draw(matrix());
  drawable.draw_translucent(matrix(), linal::double3{0.0, 0.0, 2.0});

  opengl::PointDrawable destination = make_point_drawable_with_alpha(program, opaqueColors);
  destination = std::move(drawable);
  EXPECT_TRUE(destination.has_translucent_primitives());
}

TEST_F(OpenGLDrawableTest, LineDrawableUpdatesDrawsWithoutPointPassAndMoveAssigns)
{
  opengl::LineProgram program = opengl::make_line_program();
  const std::vector<float> mixedColors = {
      1.0F, 0.0F, 0.0F, 1.0F,
      1.0F, 0.0F, 0.0F, 1.0F,
      0.0F, 1.0F, 0.0F, 1.0F,
      0.0F, 1.0F, 0.0F, 0.5F,
  };
  opengl::LineDrawable drawable = make_line_drawable_with_alpha(program, mixedColors, 0.0F);
  EXPECT_TRUE(drawable.has_opaque_primitives());
  EXPECT_TRUE(drawable.has_translucent_primitives());

  const std::vector<float> updatedVertices = {
      -1.0F, 0.0F, 0.0F,
      0.0F, 0.0F, 0.0F,
      1.0F, 0.0F, 0.0F,
      2.0F, 0.0F, 0.0F,
  };
  const std::vector<float> opaqueColors = {
      1.0F, 0.0F, 0.0F, 1.0F,
      1.0F, 0.0F, 0.0F, 1.0F,
      0.0F, 1.0F, 0.0F, 1.0F,
      0.0F, 1.0F, 0.0F, 1.0F,
  };
  const std::vector<std::uint32_t> emptyIndices;
  drawable.update_line_drawable(updatedVertices, opaqueColors, emptyIndices, opengl::BufferAccessPattern::DYNAMIC_DRAW);
  EXPECT_FALSE(drawable.has_opaque_primitives());
  EXPECT_FALSE(drawable.has_translucent_primitives());
  drawable.draw_opaque(matrix());

  const std::vector<std::uint32_t> translucentIndices = {2U, 3U};
  drawable.update_line_drawable(updatedVertices, mixedColors, translucentIndices, opengl::BufferAccessPattern::DYNAMIC_DRAW);
  EXPECT_FALSE(drawable.has_opaque_primitives());
  EXPECT_TRUE(drawable.has_translucent_primitives());
  drawable.draw(matrix());
  drawable.draw_translucent(matrix(), linal::double3{0.0, 0.0, 2.0});

  opengl::LineDrawable destination = make_line_drawable_with_alpha(program, opaqueColors, 0.0F);
  destination = std::move(drawable);
  EXPECT_TRUE(destination.has_translucent_primitives());
}

TEST_F(OpenGLDrawableTest, MeshDrawableUpdatesDrawsAndMoveAssigns)
{
  opengl::MeshProgram program = opengl::make_mesh_program();
  const std::vector<float> opaqueColors = {
      1.0F, 0.0F, 0.0F, 1.0F,
      0.0F, 1.0F, 0.0F, 1.0F,
      0.0F, 0.0F, 1.0F, 1.0F,
  };
  const std::vector<float> translucentColors = {
      1.0F, 0.0F, 0.0F, 1.0F,
      0.0F, 1.0F, 0.0F, 0.5F,
      0.0F, 0.0F, 1.0F, 1.0F,
  };

  opengl::MeshDrawable drawable = make_mesh_drawable_with_alpha(program, opaqueColors);
  EXPECT_FALSE(drawable.is_translucent());
  drawable.update_color_buffer(translucentColors, opengl::BufferAccessPattern::DYNAMIC_DRAW);
  EXPECT_TRUE(drawable.is_translucent());
  drawable.draw(matrix(),
                matrix(),
                matrix(),
                matrix(),
                linal::float3{0.0F, 0.0F, 1.0F},
                linal::float3{0.0F, 0.0F, 2.0F},
                linal::float3{1.0F, 1.0F, 1.0F},
                linal::float3{0.1F, 0.1F, 0.1F},
                8.0F);

  opengl::MeshDrawable destination = make_mesh_drawable_with_alpha(program, opaqueColors);
  destination = std::move(drawable);
  EXPECT_TRUE(destination.is_translucent());
}
