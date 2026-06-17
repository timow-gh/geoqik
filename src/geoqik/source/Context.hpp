#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <Renderer/Camera.hpp>
#include "CameraInteractor.hpp"
#include "ConcurrentQueue/ConcurrentQueue.hpp"
#include "GeoQik/GeoQik.hpp"
#include "GeoQikLog.hpp"
#include "GeoQikMessages.hpp"
#include "GeoQikReplayUndo.hpp"
#include "GeoQikSettings.hpp"
#include "IdempotencyData.hpp"
#include "Scene.hpp"
#include <Renderer/WindowSettings.hpp>
#include <Core/UUID.hpp>
#include <atomic>
#include <cassert>
#include <chrono>
#include <deque>
#include <memory>
#include <span>
#include <utility>
#include <variant>
#include <vector>
#include <Renderer/InputState.hpp>

namespace renderer
{
class GlfwWindow;
}

namespace geoqik
{

using renderer::GlfwWindow;
using renderer::Key;
using renderer::Scancode;
using renderer::Action;
using renderer::Mods;
class ImGuiOverlay;
class OpenGLSceneRenderer;

void init_message_queue(ConcurrentQueue<GeoQikMessage>&& messageQueue);
[[nodiscard]] ConcurrentQueue<GeoQikMessage>& get_message_queue();
void request_replay_cancel();

struct ReplayOptions
{
  double entriesPerSecond{60.0};
  std::size_t maxEntriesPerFrame{1024};
  bool startPaused{false};
  std::size_t entriesPerStep{1};
  std::vector<Key> stepKeys;
  std::vector<Key> backwardStepKeys;
  std::vector<Key> resumeKeys;
  std::vector<Key> pauseKeys;
  std::vector<Key> increaseEntriesPerStepKeys;
  std::vector<Key> decreaseEntriesPerStepKeys;
};

// Holds idempotency key for messages.
// If a message with the same key has already been processed, it will be ignored.


class Context
{
  GeoQikSettings m_geoqikSettings;
  std::unique_ptr<WindowSettings> m_windowSettings;

  std::unique_ptr<GlfwWindow> m_window;
  std::atomic<bool> m_windowShouldClose{false};

  std::unique_ptr<CameraInteractor> m_cameraInteractor;
  std::unique_ptr<ImGuiOverlay> m_imguiOverlay;

  Color m_backgroundColor{0.1f, 0.1f, 0.1f, 1.0f}; // Default background color

  IdempotencySet m_idempotencySet;

  Scene m_scene;
  std::unique_ptr<OpenGLSceneRenderer> m_renderer;
  bool m_isDrawing{false};
  std::size_t m_frameCount{0};
  std::size_t m_geometryMessagesProcessedThisFrame{0};
  std::vector<GeoQikLogEntry> m_messageLog;
  std::vector<GeoQikLogEntry> m_replayEntries;
  std::vector<ReplayUndoFrame> m_replayUndoStack;
  std::size_t m_replayEntryIndex{0};
  double m_replayEntryBudget{0.0};
  ReplayOptions m_replayOptions;
  bool m_isReplayActive{false};
  bool m_isReplayPaused{false};
  std::chrono::high_resolution_clock::time_point m_lastReplayTick;
  std::chrono::high_resolution_clock::time_point m_lastCameraInteractionTime;
  std::deque<GeoQikMessage> m_deferredMessages;

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
  void update_point_with_opts(const core::UUID& handle, float x, float y, float z, std::span<const float> colors);
  void update_points_with_opts(const core::UUID& handle, std::span<const float> points, std::span<const float> colors);

  void remove_point(const core::UUID& handle);

  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, const core::UUID* handle = nullptr, const core::UUID* idempotencyKey = nullptr);
  void add_line(float x1, float y1, float z1, float x2, float y2, float z2, float r, float g, float b, float a, const core::UUID* handle = nullptr,  const core::UUID* idempotencyKey = nullptr);
  void add_line_with_opts(float x1, float y1, float z1, float x2, float y2, float z2, const GeoQikMessageCommonData& commonData);
  void add_lines_with_opts(std::span<const float> lines, const GeoQikMessageCommonData& commonData);
  void update_line_with_opts(const core::UUID& handle,
                             float x1,
                             float y1,
                             float z1,
                             float x2,
                             float y2,
                             float z2,
                             std::span<const float> colors);
  void update_lines_with_opts(const core::UUID& handle, std::span<const float> lines, std::span<const float> colors);
  void remove_line(const core::UUID& handle);

  void remove_all_geometry();

  void add_mesh_with_opts(std::span<const float> vertices,
                          std::span<const float> normals,
                          std::span<const float> colors,
                          std::span<const std::uint32_t> triangleIndices,
                          const GeoQikMessageCommonData& commonData);

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
  geoqik_error_code_t replay_log(const char* path, geoqik_log_format_t format, const ReplayOptions& options);
  geoqik_error_code_t replay_current_log(const ReplayOptions& options);
  void cancel_replay();
  void pause_replay();
  void resume_replay();
  void step_replay_entries(std::size_t count);
  void step_replay_entries_backward(std::size_t count);
  [[nodiscard]] geoqik_replay_state_t get_replay_state() const;
  [[nodiscard]] std::pair<std::size_t, std::size_t> get_replay_progress() const;

  bool cleanup();

private:
  void setup_window_callbacks();
  [[nodiscard]] bool is_replaying() const;
  [[nodiscard]] static bool is_control_message(const GeoQikMessage& message);
  void on_key(Key key, Scancode scancode, Action action, Mods mods);
  [[nodiscard]] static bool has_replay_key(const std::vector<Key>& keys, Key key);
  void start_replay(std::vector<GeoQikLogEntry> entries, const ReplayOptions& options);
  void process_replay_entries(const std::chrono::high_resolution_clock::time_point& now);
  void apply_replay_entries(std::size_t entriesToApply);
  void undo_replay_entries(std::size_t entriesToUndo);
  [[nodiscard]] ReplayUndoFrame create_replay_undo_frame(const GeoQikLogEntry& entry) const;
  void restore_replay_undo_frame(const ReplayUndoFrame& frame);
  void finish_replay();
  [[nodiscard]] bool should_close_event_loop();
  void update_camera_interaction_state();
  void sync_scene_and_auto_fit();
  [[nodiscard]] bool should_stop_processing_messages(const std::chrono::high_resolution_clock::time_point& frameStartTime,
                                                     const std::chrono::high_resolution_clock::time_point& messageProcessingStartTime) const;
  [[nodiscard]] bool process_message_queue(ConcurrentQueue<GeoQikMessage>& messageQueue,
                                           const std::chrono::high_resolution_clock::time_point& frameStartTime);
  void defer_or_handle_message(GeoQikMessage&& message);
  bool process_message(const GeoQikMessage& message, bool recordLogEntry);
  bool process_deferred_messages();
  bool process_deferred_messages_before_cleanup();
  [[nodiscard]] bool is_known_idempotency_key(const core::UUID* key);
  void replay_log_entries(const std::vector<GeoQikLogEntry>& entries);
  void apply_log_entry(const GeoQikLogEntry& entry);

  void handle_message(const AddPointWithOpts& message);
  void handle_message(const AddPointsWithOpts& message);
  void handle_message(const UpdatePointWithOpts& message);
  void handle_message(const UpdatePointsWithOpts& message);
  void handle_message(const RemovePoint& message);
  void handle_message(const SetPointSize& message);
  void handle_message(const SetPointColor& message);
  void handle_message(const AddLineWithOpts& message);
  void handle_message(const AddLinesWithOpts& message);
  void handle_message(const UpdateLineWithOpts& message);
  void handle_message(const UpdateLinesWithOpts& message);
  void handle_message(const RemoveLine& message);
  void handle_message(const SetLineWidth& message);
  void handle_message(const SetLineColor& message);
  void handle_message(const RemoveAllGeometry& message);
  void handle_message(const TranslateGeometry& message);
  void handle_message(const RotateGeometry& message);
  void handle_message(const AddMeshWithOpts& message);
  void handle_message(const RemoveMesh& message);
  void handle_message(const Draw& message);
  void handle_message(const StopDraw& message);
  void handle_message(const SaveLog& message);
  void handle_message(const LoadLog& message);
  void handle_message(const ReplayLog& message);
  void handle_message(const ReplayCurrentLog& message);
  void handle_message(const PauseReplay& message);
  void handle_message(const ResumeReplay& message);
  void handle_message(const StepReplay& message);
  void handle_message(const StepReplayBackward& message);
  void handle_message(const GetReplayState& message);
  void handle_message(const GetReplayProgress& message);
  void handle_message(const GetPointSize& message);
  void handle_message(const GetPointColor& message);
  void handle_message(const GetLineWidth& message);
  void handle_message(const GetLineColor& message);
  void handle_message(const Cleanup& message);

  void print_frame_info(const std::chrono::high_resolution_clock::time_point& startTime,
                        const std::chrono::high_resolution_clock::time_point& messageProcessingStartTime,
                        const std::chrono::high_resolution_clock::time_point& endTime) const;
};

} // namespace geoqik

#endif // CONTEXT_HPP
