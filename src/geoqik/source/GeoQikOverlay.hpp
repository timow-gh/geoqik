#ifndef GEOQIKOVERLAY_HPP
#define GEOQIKOVERLAY_HPP

#include "GeoQik/GeoQik.hpp"

#include <plinth/Camera.hpp>
#include <plinth/CameraInteractor.hpp>
#include <plinth/CameraProjectionType.hpp>
#include <plinth/IOverlay.hpp>
#include <plinth/ImGuiOverlay.hpp>
#include <plinth/InputState.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace geoqik {

struct ReplayGuiState {
    bool isActive{false};
    bool isPaused{false};
    bool isBackward{false};
    std::size_t currentEntry{0};
    std::size_t totalEntries{0};
    double speedMultiplier{1.0};
    std::size_t entriesPerStep{1};
    std::string pauseKeysLabel;
    std::string resumeKeysLabel;
    std::string stepForwardKeysLabel;
    std::string stepBackwardKeysLabel;
    std::string increaseStepKeysLabel;
    std::string decreaseStepKeysLabel;

    enum class Command : std::uint8_t {
        None,
        Play,
        PlayReverse,
        Pause,
        SkipToEnd,
        EndReplay,
        StepForward,
        StepBackward,
    };
    Command command{Command::None};
    std::optional<double> requestedSpeedMultiplier;
    std::optional<std::size_t> requestedEntriesPerStep;
    std::optional<std::size_t> requestedEntry;
};

struct CameraGuiState {
    renderer::CameraInteractor::NavigationStyle navigationStyle{renderer::CameraInteractor::NavigationStyle::ORBIT};
    std::optional<renderer::PresetView> activePreset;
    bool autoZoom{false};
    renderer::CameraProjectionType projectionType{renderer::CameraProjectionType::PERSPECTIVE};
    bool replayWasActive{false};
    std::optional<renderer::CameraInteractor::NavigationStyle> requestedNavigationStyle;
    std::optional<renderer::PresetView> requestedPreset;
    std::optional<renderer::CameraProjectionType> requestedProjection;
    bool requestHome{false};
};

struct FileGuiState {
    enum class DialogRequest : std::uint8_t {
        None,
        SaveBinary,
        SaveJson,
        Load,
        SelectDefaultDirectory
    };
    enum class Command : std::uint8_t {
        None,
        Save,
        Load,
        SetDefaultDirectory
    };

    bool nativeDialogsInitialized{false};
    std::filesystem::path settingsFilePath;
    std::filesystem::path defaultLogDirectory;
    std::filesystem::path requestedPath;
    geoqik_log_format_t requestedFormat{GEOQIK_LOG_FORMAT_BINARY};
    DialogRequest dialogRequest{DialogRequest::None};
    Command command{Command::None};
    std::string errorMessage;
    bool openErrorPopup{false};
};

// GeoQik owns its controls and docked sidebar. Plinth's ImGuiOverlay is retained only as the
// platform/backend adapter that owns the ImGui context and translates window input.
class GeoQikOverlay final : public renderer::IOverlay {
    renderer::ImGuiOverlay m_inner;
    std::vector<std::function<void()>> m_controls;
    CameraGuiState m_cameraState;
    ReplayGuiState m_replayState;
    FileGuiState m_fileState;
    float m_controlPanelWidth{320.0F};
    renderer::UiMode m_uiMode{renderer::UiMode::Release};

  public:
    explicit GeoQikOverlay(void* nativeWindow);
    ~GeoQikOverlay() override;

    void add_control(std::function<void()> control) { m_controls.emplace_back(std::move(control)); }
    void add_replay_controls();
    void add_camera_controls();
    void add_display_controls(renderer::Renderer& renderer);
    void apply_scene_viewport_hint(renderer::OverlayFrameContext& context) const;
    void shutdown_native_dialogs();
    [[nodiscard]] CameraGuiState& camera_state() { return m_cameraState; }
    [[nodiscard]] ReplayGuiState& replay_state() { return m_replayState; }
    [[nodiscard]] FileGuiState& file_state() { return m_fileState; }
    [[nodiscard]] float control_panel_width() const { return m_controlPanelWidth; }
    void set_control_panel_width(float width) { m_controlPanelWidth = width; }
    [[nodiscard]] renderer::UiMode ui_mode() const { return m_uiMode; }
    void set_ui_mode(renderer::UiMode mode) { m_uiMode = mode; }

    void new_frame() override { m_inner.new_frame(); }

    void build_controls(renderer::OverlayFrameContext& context) override;

    void render() override;
    void end_frame() override { m_inner.end_frame(); }

    [[nodiscard]] bool wants_mouse() const override { return m_inner.wants_mouse(); }
    [[nodiscard]] bool wants_keyboard() const override { return m_inner.wants_keyboard(); }

    [[nodiscard]] bool handle_cursor_position(double xpos, double ypos) override {
        return m_inner.handle_cursor_position(xpos, ypos);
    }
    [[nodiscard]] bool handle_mouse_button(int button, renderer::Action action, renderer::Mods mods) override {
        return m_inner.handle_mouse_button(button, action, mods);
    }
    [[nodiscard]] bool handle_scroll(double xoffset, double yoffset) override {
        return m_inner.handle_scroll(xoffset, yoffset);
    }
    [[nodiscard]] bool
    handle_key(renderer::Key key, renderer::Scancode scancode, renderer::Action action, renderer::Mods mods) override {
        return m_inner.handle_key(key, scancode, action, mods);
    }
    void handle_char(std::uint32_t codepoint) override { m_inner.handle_char(codepoint); }

  private:
    void render_main_menu_bar();
    void layout_controls();
};

} // namespace geoqik

#endif // GEOQIKOVERLAY_HPP
