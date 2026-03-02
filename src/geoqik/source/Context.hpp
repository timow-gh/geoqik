#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "Camera.hpp"
#include "CameraInteractor.hpp"
#include "ConcurrentQueue/ConcurrentQueue.hpp"
#include "GeoQikMessages.hpp"
#include "GeoQikSettings.hpp"
#include "GeoQik_Glfw.hpp"
#include "Scene.hpp"
#include "WindowSettings.hpp"
#include <Core/UUID.hpp>
#include <OpenGL/Programs/ProgramManager.hpp>
#include <atomic>
#include <cassert>
#include <chrono>
#include <compare>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace geoqik
{

void init_message_queue(ConcurrentQueue<GeoQikMessage>&& messageQueue);
[[nodiscard]] ConcurrentQueue<GeoQikMessage>& get_message_queue();

// Holds idempotency key for messages.
// If a message with the same key has already been processed, it will be ignored.
struct IdempotencyData
{
  core::UUID key;
  std::chrono::high_resolution_clock::time_point timestamp;

  [[nodiscard]] bool operator==(const IdempotencyData& other) const noexcept
  {
    return key == other.key;
  }
  [[nodiscard]] bool operator!=(const IdempotencyData& other) const noexcept
  {
    return key != other.key;
  }

  [[nodiscard]] auto operator<(const IdempotencyData& other) const noexcept
  {
    return key < other.key;
  }
  
  // Add hash function as a member
  struct Hash
  {
    std::size_t operator()(const IdempotencyData& data) const noexcept
    {
      return std::hash<core::UUID>{}(data.key);
    }
  };
};

class Context
{
  GeoQikSettings m_geoqikSettings;
  std::unique_ptr<WindowSettings> m_windowSettings;

  GLFWwindow* m_glfwWindow = nullptr;

  opengl::ProgramManager m_programManager;
  std::atomic<bool> m_windowShouldClose{false};

  std::unique_ptr<CameraInteractor> m_cameraInteractor;

  std::array<float, 4> m_backgroundColor{0.1f, 0.1f, 0.1f, 1.0f}; // Default background color

  std::unordered_set<IdempotencyData, IdempotencyData::Hash> m_idempotencySet;

  Scene m_scene;
  bool m_isDrawing{false};
  std::size_t m_frameCount{0};
  std::size_t m_drawAtEveryNGeometryChanges{10};
  std::size_t m_geometryMessagesProcessedThisFrame{0};
  float m_drawAtEveryNPercentOfMessages{10.0f};

  using MessageHandler = std::function<void(Context&, const GeoQikMessage&)>;
  std::unordered_map<GeoQikMessageType, MessageHandler> m_messageHandlers;

public:
  Context() = default;

  static std::unique_ptr<Context> create(const GeoQikSettings& geoqikSettings, const WindowSettings& settings)
  {
    auto context = std::make_unique<Context>();
    context->m_geoqikSettings = geoqikSettings;
    context->m_windowSettings = std::make_unique<WindowSettings>(settings);
    return context;
  }

  bool init_window(const GeoQikSettings& geoqikSettings, const WindowSettings& settings);

  float get_point_size();
  void set_point_size(float pointSize);

  std::array<float, 3> get_point_color();
  void set_point_color(std::array<float, 3> color);

  float get_line_width();
  void set_line_width(float lineWidth);

  std::array<float, 3> get_line_color();
  void set_line_color(std::array<float, 3> color);

  void add_point(float x, float y, float z, const core::UUID* handle = nullptr, const core::UUID* idempotencyKey = nullptr);
  void add_point(float x, float y, float z, float r, float g, float b, const core::UUID* handle = nullptr, const core::UUID* idempotencyKey = nullptr);
  void remove_point(const core::UUID& handle);

  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, const core::UUID* handle = nullptr, const core::UUID* idempotencyKey = nullptr);
  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, float r, float g, float b, const core::UUID* handle = nullptr,  const core::UUID* idempotencyKey = nullptr);
  void remove_line(const core::UUID& handle);

  void remove_all_geometry();

  [[nodiscard]] const Viewport& get_viewport();

  void run_event_loop();

  bool cleanup();

private:
  [[nodiscard]] static int window_hint_to_glfw_hint(bool hint) { return hint ? 1 : 0; }
  void set_window_hints(const WindowSettings& hints);

  void initialize_message_handlers();

  [[nodiscard]] bool is_known_idempotency_key(const core::UUID* key);

  [[nodiscard]] opengl::ProgramManager& get_program_manager();
  [[nodiscard]] opengl::PointProgram& get_point_program() { return m_programManager.get_point_program(); }
  [[nodiscard]] opengl::LineProgram& get_line_program() { return m_programManager.get_line_program(); }

  void print_frame_info(const std::chrono::high_resolution_clock::time_point& startTime,
                        const std::chrono::high_resolution_clock::time_point& messageProcessingEndTime,
                        const std::chrono::high_resolution_clock::time_point& endTime) const;
};

} // namespace geoqik

#endif // CONTEXT_HPP