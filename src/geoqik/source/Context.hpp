#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "Camera.hpp"
#include "CameraInteractor.hpp"
#include "ConcurrentQueue/ConcurrentQueue.hpp"
#include "GeoQik/GeoQik.hpp"
#include "GeoQikLog.hpp"
#include "GeoQikMessages.hpp"
#include "GeoQikSettings.hpp"
#include "IdempotencyData.hpp"
#include "Scene.hpp"
#include "WindowSettings.hpp"
#include <Core/UUID.hpp>
#include <atomic>
#include <cassert>
#include <chrono>
#include <memory>
#include <span>
#include <unordered_set>
#include <vector>

namespace geoqik
{

class GlfwWindow;
class OpenGLSceneRenderer;

void init_message_queue(ConcurrentQueue<GeoQikMessage>&& messageQueue);
[[nodiscard]] ConcurrentQueue<GeoQikMessage>& get_message_queue();

// Holds idempotency key for messages.
// If a message with the same key has already been processed, it will be ignored.


class Context
{
  GeoQikSettings m_geoqikSettings;
  std::unique_ptr<WindowSettings> m_windowSettings;

  std::unique_ptr<GlfwWindow> m_window;
  std::atomic<bool> m_windowShouldClose{false};

  std::unique_ptr<CameraInteractor> m_cameraInteractor;

  Color m_backgroundColor{0.1f, 0.1f, 0.1f, 1.0f}; // Default background color

  std::unordered_set<IdempotencyData, IdempotencyData::Hash> m_idempotencySet;

  Scene m_scene;
  std::unique_ptr<OpenGLSceneRenderer> m_renderer;
  bool m_isDrawing{false};
  std::size_t m_frameCount{0};
  std::size_t m_drawAtEveryNGeometryChanges{10};
  std::size_t m_geometryMessagesProcessedThisFrame{0};
  float m_drawAtEveryNPercentOfMessages{10.0f};
  std::vector<GeoQikLogEntry> m_messageLog;

public:
  Context();
  ~Context();

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

  Color get_point_color();
  void set_default_point_color(Color color);

  float get_line_width();
  void set_line_width(float lineWidth);

  Color get_line_color();
  void set_line_color(Color color);

  void add_point_with_opts(float x, float y, float z, const GeoQikMessageCommonData& commonData);
  void add_points_with_opts(std::span<const float> points, const GeoQikMessageCommonData& commonData);

  void remove_point(const core::UUID& handle);

  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, const core::UUID* handle = nullptr, const core::UUID* idempotencyKey = nullptr);
  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, float r, float g, float b, float a, const core::UUID* handle = nullptr,  const core::UUID* idempotencyKey = nullptr);
  void add_line_with_opts(float x1, float y1, float z1, float x2, float y2, float z2, const GeoQikMessageCommonData& commonData);
  void add_lines_with_opts(std::span<const float> lines, const GeoQikMessageCommonData& commonData);
  void remove_line(const core::UUID& handle);

  void remove_all_geometry();

  void translate_geometry(const core::UUID& handle, float dx, float dy, float dz);
  void rotate_geometry(const core::UUID& handle,
                       float centerX,
                       float centerY,
                       float centerZ,
                       float axisX,
                       float axisY,
                       float axisZ,
                       float angle);

  [[nodiscard]] const Viewport& get_viewport();

  void run_event_loop();

  geoqik_error_code_t save_log(const char* path, geoqik_log_format_t format) const;
  geoqik_error_code_t load_log(const char* path, geoqik_log_format_t format);

  bool cleanup();

private:
  [[nodiscard]] bool is_known_idempotency_key(const core::UUID* key);
  void replay_log_entries(const std::vector<GeoQikLogEntry>& entries);
  void apply_log_entry(const GeoQikLogEntry& entry);

  void handle_message(const AddPointWithOpts& message);
  void handle_message(const AddPointsWithOpts& message);
  void handle_message(const RemovePoint& message);
  void handle_message(const SetPointSize& message);
  void handle_message(const SetPointColor& message);
  void handle_message(const AddLineWithOpts& message);
  void handle_message(const AddLinesWithOpts& message);
  void handle_message(const RemoveLine& message);
  void handle_message(const SetLineWidth& message);
  void handle_message(const SetLineColor& message);
  void handle_message(const RemoveAllGeometry& message);
  void handle_message(const TranslateGeometry& message);
  void handle_message(const RotateGeometry& message);
  void handle_message(const Draw& message);
  void handle_message(const StopDraw& message);
  void handle_message(const SaveLog& message);
  void handle_message(const LoadLog& message);
  void handle_message(const GetPointSize& message);
  void handle_message(const GetPointColor& message);
  void handle_message(const GetLineWidth& message);
  void handle_message(const GetLineColor& message);
  void handle_message(const Cleanup& message);

  void print_frame_info(const std::chrono::high_resolution_clock::time_point& startTime,
                        const std::chrono::high_resolution_clock::time_point& messageProcessingEndTime,
                        const std::chrono::high_resolution_clock::time_point& endTime) const;
};

} // namespace geoqik

#endif // CONTEXT_HPP
