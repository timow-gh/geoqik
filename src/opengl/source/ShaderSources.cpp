#include "OpenGL/ShaderSources.hpp"
#include "OpenGL/FmtIncludeHelper.hpp"

namespace opengl
{
std::string line_vertex_shader_source() {
  return
      R"(#version 430

uniform mat4 u_MVP;

in vec3 a_vertex;
in vec3 a_color;

out vec4 v_color;

void main() {
    gl_Position = u_MVP * vec4(a_vertex, 1.0);
    v_color = vec4(a_color, 1.0);
})";
}

std::string line_fragment_shader_source() {
  return
      R"(#version 430

in vec4 v_color;

out vec4 FragColor;

void main() {
    FragColor = v_color;
}
)";
}
std::string point_color_vertex_shader_source() {
  return
      R"(#version 430

uniform mat4 u_MVP;
uniform float u_pointSize;

in vec3 a_vertex;
in vec4 a_color;

out vec4 v_color;

void main() {
    gl_Position = u_MVP * vec4(a_vertex, 1.0);
    gl_PointSize = u_pointSize;
    v_color = vec4(a_color.rgb, 1.0);
})";
}
std::string point_color_fragment_shader_source() {
  return
      R"(#version 430

in vec4 v_color;

out vec4 FragColor;

void main() {
    FragColor = v_color;
})";
}

std::string mesh_vertex_shader_source() {
  return
      R"(#version 430

    uniform mat4 u_model;
    uniform mat4 u_view;
    uniform mat4 u_projection;
    uniform mat4 u_normalMatrix;

    in vec3 a_vertex;
    in vec4 a_color;
    in vec3 a_normal;

    out vec4 v_color;
    out vec3 v_normal;
    out vec3 v_position;

    void main() {
        gl_Position = u_projection * u_view * u_model * vec4(a_vertex, 1.0);
        v_color = vec4(a_color.rgb, 1.0);
        v_normal = mat3(u_normalMatrix) * a_normal;
        v_position = vec3(u_model * vec4(a_vertex, 1.0));
    })";
}

std::string mesh_fragment_shader_source() {
  return
      R"(#version 430

    in vec4 v_color;
    in vec3 v_normal;
    in vec3 v_position;

    out vec4 FragColor;

    uniform vec3 u_lightPos;
    // The view position is the position of the camera in world space.
    // It is used to calculate the view direction in the fragment shader.
    uniform vec3 u_viewPos;
    uniform vec3 u_lightColor;
    uniform vec3 u_ambientColor;
    uniform float u_shininess;

    void main() {
        // Normalize input vectors
        vec3 norm = normalize(v_normal);
        vec3 lightDir = normalize(u_lightPos - v_position);
        vec3 viewDir = normalize(u_viewPos - v_position);

        // Ambient lighting
        vec3 ambient = u_ambientColor * v_color.rgb;

        // Diffuse lighting
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = u_lightColor * diff * v_color.rgb;

        // Specular lighting
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_shininess);
        vec3 specular = u_lightColor * spec;

        // Combine results
        vec3 result = ambient + diffuse + specular;
        FragColor = vec4(result, 1.0);
    })";
}

} // namespace opengl
