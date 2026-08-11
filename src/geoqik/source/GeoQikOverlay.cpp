#include "GeoQikOverlay.hpp"

#include "Core/FmtIncludeHelper.hpp"
#include "GeoQikUserSettings.hpp"

#include <plinth/LogicalViewportRect.hpp>
#include <plinth/Renderer.hpp>
#include <plinth/Warnings.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <exception>
#include <string>
#include <string_view>
#include <utility>

RENDERER_DISABLE_ALL_WARNINGS
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <nfd.h>
RENDERER_ENABLE_ALL_WARNINGS

namespace geoqik {

namespace {

constexpr float controlPanelMinWidth = 280.0F;
constexpr float controlPanelMaxWidth = 480.0F;
constexpr float resizeGripWidth = 8.0F;
constexpr float tooltipWrapWidth = 360.0F;
constexpr float autoFitColumnStretch = 0.8F;
constexpr float projectionColumnStretch = 1.2F;
constexpr float hdrDisplayMinimum = 0.1F;
constexpr float hdrDisplayMaximum = 100.0F;
constexpr float fxaaEdgeThresholdMaximum = 0.5F;
constexpr float fxaaMinimumEdgeContrastMaximum = 0.25F;
constexpr float errorPopupButtonWidth = 120.0F;
constexpr float exposureMinimum = -10.0F;
constexpr float exposureMaximum = 10.0F;

// Recording status/badge presentation.
constexpr double recordingStatusTimeoutSeconds = 4.0; // inline status fades after this long
constexpr float recordingBadgePadding = 8.0F;
constexpr float recordingBadgeMargin = 12.0F;
constexpr float recordingBadgeRounding = 4.0F;
// Field widths reused by the record menu and log->video settings.
constexpr float logVideoFieldWidth = 200.0F;
constexpr float recordQualityFieldWidth = 160.0F;
// Frame-rate bounds for the log->video FPS slider.
constexpr int logVideoMinFps = 15;
constexpr int logVideoMaxFps = 120;

/// ImGui text color for a recording status message. None falls back to the default text color.
[[nodiscard]] ImU32 recording_status_color(VideoGuiState::StatusKind kind) {
    switch (kind) {
    case VideoGuiState::StatusKind::Success: return IM_COL32(96, 208, 96, 255);
    case VideoGuiState::StatusKind::Error: return IM_COL32(240, 96, 96, 255);
    case VideoGuiState::StatusKind::Info:
    case VideoGuiState::StatusKind::None: return ImGui::GetColorU32(ImGuiCol_Text);
    }
    return ImGui::GetColorU32(ImGuiCol_Text);
}

// Standard video resolution presets offered for log->video rendering. Index 0 keeps the current
// window size; the rest resize the window during the offline render. Shared by the menu UI and by
// Context::make_record_options_from_gui via video_resolution_preset().
struct ResolutionPreset {
    const char* label;
    int width;  // 0 = current window size
    int height;
};
constexpr std::array<ResolutionPreset, 5> videoResolutionPresets{{
    {"Window size", 0, 0},
    {"720p (1280x720)", 1280, 720},
    {"1080p (1920x1080)", 1920, 1080},
    {"1440p (2560x1440)", 2560, 1440},
    {"4K (3840x2160)", 3840, 2160},
}};

std::string path_to_utf8(const std::filesystem::path& path) {
    const std::u8string value = path.u8string();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return {reinterpret_cast<const char*>(value.data()), value.size()};
}

std::filesystem::path path_from_utf8(const char* value) {
    const std::string_view bytes{value};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto* begin = reinterpret_cast<const char8_t*>(bytes.data());
    return std::filesystem::path{std::u8string{begin, begin + bytes.size()}};
}

std::string lowercase_extension(const std::filesystem::path& path) {
    std::string extension = path.extension().string();
    std::ranges::transform(extension, extension.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return extension;
}

void item_tooltip(const std::string& text) {
    if (!text.empty() && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(tooltipWrapWidth);
        ImGui::TextUnformatted(text.c_str());
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

float equal_button_width(int buttonCount) {
    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float spacing = ImGui::GetStyle().ItemSpacing.x * static_cast<float>(buttonCount - 1);
    return std::max(1.0F, (availableWidth - spacing) / static_cast<float>(buttonCount));
}

bool full_width_button(const char* label) {
    return ImGui::Button(label, ImVec2{-1.0F, 0.0F});
}

bool equal_width_button(const char* label, float width) {
    return ImGui::Button(label, ImVec2{width, 0.0F});
}

bool highlighted_button(const char* label, float width, bool isActive) {
    if (isActive) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    }
    const bool clicked = equal_width_button(label, width);
    if (isActive) {
        ImGui::PopStyleColor();
    }
    return clicked;
}

void set_file_dialog_error(FileGuiState& state, const std::string& prefix) {
    const char* detail = NFD_GetError();
    state.errorMessage = detail == nullptr ? prefix : fmt::format("{}: {}", prefix, detail);
    state.openErrorPopup = true;
}

void select_save_log_path(FileGuiState& state, geoqik_log_format_t format) {
    const bool isJson = format == GEOQIK_LOG_FORMAT_JSON;
    const nfdu8filteritem_t filter = {isJson ? "JSON log" : "GeoQik binary log", isJson ? "json" : "gqklog"};
    const std::string directory = path_to_utf8(state.defaultLogDirectory);
    const char* defaultName = isJson ? "geoqik-log.json" : "geoqik-log.gqklog";
    nfdsavedialogu8args_t arguments{};
    arguments.filterList = &filter;
    arguments.filterCount = 1;
    arguments.defaultPath = directory.empty() ? nullptr : directory.c_str();
    arguments.defaultName = defaultName;

    nfdu8char_t* selectedPath = nullptr;
    const nfdresult_t result = NFD_SaveDialogU8_With(&selectedPath, &arguments);
    if (result == NFD_OKAY) {
        state.requestedPath = path_from_utf8(selectedPath);
        NFD_FreePathU8(selectedPath);
        state.requestedPath.replace_extension(isJson ? ".json" : ".gqklog");
        state.requestedFormat = format;
        state.command = FileGuiState::Command::Save;
    } else if (result == NFD_ERROR) {
        set_file_dialog_error(state, "Could not open the save dialog");
    }
}

void select_log_file(FileGuiState& state, FileGuiState::Command command) {
    constexpr std::array<nfdu8filteritem_t, 2> filters{nfdu8filteritem_t{"GeoQik binary log", "gqklog"},
                                                       nfdu8filteritem_t{"JSON log", "json"}};
    const std::string directory = path_to_utf8(state.defaultLogDirectory);
    nfdopendialogu8args_t arguments{};
    arguments.filterList = filters.data();
    arguments.filterCount = static_cast<nfdfiltersize_t>(filters.size());
    arguments.defaultPath = directory.empty() ? nullptr : directory.c_str();

    nfdu8char_t* selectedPath = nullptr;
    const nfdresult_t result = NFD_OpenDialogU8_With(&selectedPath, &arguments);
    if (result == NFD_OKAY) {
        state.requestedPath = path_from_utf8(selectedPath);
        NFD_FreePathU8(selectedPath);
        state.requestedFormat =
            lowercase_extension(state.requestedPath) == ".json" ? GEOQIK_LOG_FORMAT_JSON : GEOQIK_LOG_FORMAT_BINARY;
        state.command = command;
    } else if (result == NFD_ERROR) {
        set_file_dialog_error(state, "Could not open the log file dialog");
    }
}

void select_default_log_directory(FileGuiState& state) {
    const std::string directory = path_to_utf8(state.defaultLogDirectory);
    nfdpickfolderu8args_t arguments{};
    arguments.defaultPath = directory.empty() ? nullptr : directory.c_str();

    nfdu8char_t* selectedPath = nullptr;
    const nfdresult_t result = NFD_PickFolderU8_With(&selectedPath, &arguments);
    if (result == NFD_OKAY) {
        state.requestedPath = path_from_utf8(selectedPath);
        NFD_FreePathU8(selectedPath);
        state.command = FileGuiState::Command::SetDefaultDirectory;
    } else if (result == NFD_ERROR) {
        set_file_dialog_error(state, "Could not open the folder dialog");
    }
}

void set_video_dialog_error(VideoGuiState& state, FileGuiState& fileState, const std::string& prefix) {
    // Video dialogs reuse the shared file error popup for consistency.
    (void)state;
    set_file_dialog_error(fileState, prefix);
}

void select_ffmpeg_executable(VideoGuiState& state, FileGuiState& fileState) {
#ifdef _WIN32
    const nfdu8filteritem_t filter = {"ffmpeg executable", "exe"};
    nfdopendialogu8args_t arguments{};
    arguments.filterList = &filter;
    arguments.filterCount = 1;
#else
    nfdopendialogu8args_t arguments{};
#endif
    const std::string directory = path_to_utf8(state.ffmpegPath.has_parent_path() ? state.ffmpegPath.parent_path()
                                                                                  : std::filesystem::path{});
    arguments.defaultPath = directory.empty() ? nullptr : directory.c_str();

    nfdu8char_t* selectedPath = nullptr;
    const nfdresult_t result = NFD_OpenDialogU8_With(&selectedPath, &arguments);
    if (result == NFD_OKAY) {
        state.requestedPath = path_from_utf8(selectedPath);
        NFD_FreePathU8(selectedPath);
        state.command = VideoGuiState::Command::SetFfmpegPath;
    } else if (result == NFD_ERROR) {
        set_video_dialog_error(state, fileState, "Could not open the ffmpeg file dialog");
    }
}

void select_recording_directory(VideoGuiState& state, FileGuiState& fileState) {
    const std::string directory = path_to_utf8(state.recordingDirectory);
    nfdpickfolderu8args_t arguments{};
    arguments.defaultPath = directory.empty() ? nullptr : directory.c_str();

    nfdu8char_t* selectedPath = nullptr;
    const nfdresult_t result = NFD_PickFolderU8_With(&selectedPath, &arguments);
    if (result == NFD_OKAY) {
        state.requestedPath = path_from_utf8(selectedPath);
        NFD_FreePathU8(selectedPath);
        state.command = VideoGuiState::Command::SetRecordingDirectory;
    } else if (result == NFD_ERROR) {
        set_video_dialog_error(state, fileState, "Could not open the folder dialog");
    }
}

void select_log_for_video(VideoGuiState& state, FileGuiState& fileState) {
    constexpr std::array<nfdu8filteritem_t, 2> filters{nfdu8filteritem_t{"GeoQik binary log", "gqklog"},
                                                       nfdu8filteritem_t{"JSON log", "json"}};
    const std::string directory = path_to_utf8(state.recordingDirectory);
    nfdopendialogu8args_t arguments{};
    arguments.filterList = filters.data();
    arguments.filterCount = static_cast<nfdfiltersize_t>(filters.size());
    arguments.defaultPath = directory.empty() ? nullptr : directory.c_str();

    nfdu8char_t* selectedPath = nullptr;
    const nfdresult_t result = NFD_OpenDialogU8_With(&selectedPath, &arguments);
    if (result == NFD_OKAY) {
        state.requestedPath = path_from_utf8(selectedPath);
        NFD_FreePathU8(selectedPath);
        state.requestedLogFormat =
            lowercase_extension(state.requestedPath) == ".json" ? GEOQIK_LOG_FORMAT_JSON : GEOQIK_LOG_FORMAT_BINARY;
        state.command = VideoGuiState::Command::RenderLogToVideo;
    } else if (result == NFD_ERROR) {
        set_video_dialog_error(state, fileState, "Could not open the log file dialog");
    }
}

void render_camera_controls(CameraGuiState& state, bool replayActive) {
    if (replayActive != state.replayWasActive) {
        ImGui::SetNextItemOpen(!replayActive, ImGuiCond_Always);
        state.replayWasActive = replayActive;
    }
    if (!ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    if (full_width_button("Fit scene")) {
        state.requestHome = true;
    }
    item_tooltip("Fit all geometry while preserving the current viewing direction.");
    ImGui::Spacing();

    constexpr std::array<const char*, 2> projectionItems{"Perspective", "Orthographic"};
    int projectionItem = static_cast<int>(state.projectionType);
    if (ImGui::BeginTable("##CameraOptions", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings)) {
        ImGui::TableSetupColumn("##AutoFitColumn", ImGuiTableColumnFlags_WidthStretch, autoFitColumnStretch);
        ImGui::TableSetupColumn("##ProjectionColumn", ImGuiTableColumnFlags_WidthStretch, projectionColumnStretch);
        ImGui::TableNextColumn();
        ImGui::Checkbox("Auto Fit", &state.autoZoom);
        item_tooltip("Keep the scene fitted as geometry changes.");
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1.0F);
        if (ImGui::Combo("##Projection",
                         &projectionItem,
                         projectionItems.data(),
                         static_cast<int>(projectionItems.size()))) {
            state.requestedProjection = static_cast<renderer::CameraProjectionType>(projectionItem);
        }
        item_tooltip("Camera projection.");
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::TextUnformatted("Navigation");
    const bool isFree = !state.activePreset.has_value();
    const float navigationButtonWidth = equal_button_width(2);
    const bool orbitActive = isFree && state.navigationStyle == renderer::CameraInteractor::NavigationStyle::ORBIT;
    if (highlighted_button("Orbit", navigationButtonWidth, orbitActive)) {
        state.requestedNavigationStyle = renderer::CameraInteractor::NavigationStyle::ORBIT;
    }
    ImGui::SameLine();
    const bool flyActive = isFree && state.navigationStyle == renderer::CameraInteractor::NavigationStyle::FLY;
    if (highlighted_button("Fly", navigationButtonWidth, flyActive)) {
        state.requestedNavigationStyle = renderer::CameraInteractor::NavigationStyle::FLY;
    }

    ImGui::Spacing();
    ImGui::TextUnformatted("Preset view");
    struct PresetButton {
        const char* label;
        renderer::PresetView view;
    };
    constexpr std::array<PresetButton, 7> presets{{
        {"Front", renderer::PresetView::FRONT},
        {"Back", renderer::PresetView::BACK},
        {"Left", renderer::PresetView::LEFT},
        {"Right", renderer::PresetView::RIGHT},
        {"Top", renderer::PresetView::TOP},
        {"Bottom", renderer::PresetView::BOTTOM},
        {"Iso", renderer::PresetView::ISO},
    }};
    const int presetsPerRow = ImGui::GetContentRegionAvail().x >= 360.0F ? 4 : 3;
    for (std::size_t rowStart = 0; rowStart < presets.size();) {
        const int rowButtonCount = std::min(presetsPerRow, static_cast<int>(presets.size() - rowStart));
        const float presetButtonWidth = equal_button_width(rowButtonCount);
        for (int column = 0; column < rowButtonCount; ++column) {
            if (column != 0) {
                ImGui::SameLine();
            }
            const std::size_t index = rowStart + static_cast<std::size_t>(column);
            const bool active = state.activePreset.has_value() && *state.activePreset == presets[index].view;
            if (highlighted_button(presets[index].label, presetButtonWidth, active)) {
                state.requestedPreset = presets[index].view;
            }
            item_tooltip(fmt::format("Preset shortcut: {}", index + 1));
        }
        rowStart += static_cast<std::size_t>(rowButtonCount);
    }
}

void render_replay_speed_controls(ReplayGuiState& state) {
    constexpr std::array<double, 4> speedOptions{1.0, 2.0, 4.0, 8.0};
    constexpr std::array<const char*, 4> speedLabels{"1x", "2x", "4x", "8x"};

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Speed");
    ImGui::SameLine();
    const float buttonWidth = equal_button_width(static_cast<int>(speedOptions.size()) + 1);
    for (std::size_t index = 0; index < speedOptions.size(); ++index) {
        if (index != 0) {
            ImGui::SameLine();
        }
        const bool current = !state.isMaxSpeed && std::abs(state.speedMultiplier - speedOptions[index]) < 0.01;
        if (highlighted_button(speedLabels[index], buttonWidth, current)) {
            state.requestedSpeedMultiplier = speedOptions[index];
        }
    }
    ImGui::SameLine();
    if (highlighted_button("Max", buttonWidth, state.isMaxSpeed)) {
        state.requestMaxSpeed = true;
    }
    item_tooltip("Process as many entries per frame as the replay limit allows.");
}

void render_replay_transport_controls(ReplayGuiState& state) {
    const bool canStepBack = state.currentEntry > 0;
    const bool canStepForward = state.currentEntry < state.totalEntries;
    const bool playing = !state.isPaused;

    ImGui::TextUnformatted("Playback");
    const float buttonWidth = equal_button_width(3);
    if (!canStepBack) {
        ImGui::BeginDisabled();
    }
    if (highlighted_button("Reverse", buttonWidth, playing && state.isBackward)) {
        state.command = ReplayGuiState::Command::PlayReverse;
    }
    item_tooltip("Play backward.");
    if (!canStepBack) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (!playing) {
        ImGui::BeginDisabled();
    }
    if (equal_width_button("Pause", buttonWidth)) {
        state.command = ReplayGuiState::Command::Pause;
    }
    item_tooltip(fmt::format("Pause replay ({})", state.pauseKeysLabel));
    if (!playing) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (!canStepForward) {
        ImGui::BeginDisabled();
    }
    if (highlighted_button("Play##Playback", buttonWidth, playing && !state.isBackward)) {
        state.command = ReplayGuiState::Command::Play;
    }
    item_tooltip(fmt::format("Play forward ({})", state.resumeKeysLabel));
    if (!canStepForward) {
        ImGui::EndDisabled();
    }

    render_replay_speed_controls(state);
    ImGui::Spacing();
    ImGui::TextUnformatted("Step");
    const float stepButtonWidth = equal_button_width(2);
    if (!canStepBack) {
        ImGui::BeginDisabled();
    }
    if (equal_width_button("Backward", stepButtonWidth)) {
        state.command = ReplayGuiState::Command::StepBackward;
    }
    item_tooltip(fmt::format("Step backward ({})", state.stepBackwardKeysLabel));
    if (!canStepBack) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (!canStepForward) {
        ImGui::BeginDisabled();
    }
    if (equal_width_button("Forward##Step", stepButtonWidth)) {
        state.command = ReplayGuiState::Command::StepForward;
    }
    item_tooltip(fmt::format("Step forward ({})", state.stepForwardKeysLabel));
    if (!canStepForward) {
        ImGui::EndDisabled();
    }
}

void render_replay_controls(ReplayGuiState& state) {
    if (!state.isActive || !ImGui::CollapsingHeader("Replay", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    const char* status = "Playing";
    if (state.isPaused) {
        status = "Paused";
    } else if (state.isBackward) {
        status = "Reversing";
    }
    ImGui::TextUnformatted(fmt::format("{}  -  {} / {}", status, state.currentEntry, state.totalEntries).c_str());

    auto position = static_cast<std::uint64_t>(state.currentEntry);
    constexpr std::uint64_t firstEntry = 0;
    const auto lastEntry = static_cast<std::uint64_t>(state.totalEntries);
    ImGui::SetNextItemWidth(-1.0F);
    if (ImGui::SliderScalar("##ReplayPosition",
                            ImGuiDataType_U64,
                            &position,
                            &firstEntry,
                            &lastEntry,
                            "%llu",
                            ImGuiSliderFlags_AlwaysClamp)) {
        state.requestedEntry = static_cast<std::size_t>(position);
    }
    item_tooltip("Drag to seek. Seeking pauses replay.");
    render_replay_transport_controls(state);

    const std::uint64_t maximumStepSize = std::max<std::uint64_t>(1, state.totalEntries);
    std::uint64_t stepSize = std::clamp<std::uint64_t>(state.entriesPerStep, 1, maximumStepSize);
    constexpr std::uint64_t stepIncrement = 1;
    constexpr std::uint64_t fastStepIncrement = 10;
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Step size");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0F);
    if (ImGui::InputScalar("##EntriesPerStep",
                           ImGuiDataType_U64,
                           &stepSize,
                           &stepIncrement,
                           &fastStepIncrement,
                           "%llu")) {
        state.requestedEntriesPerStep =
            static_cast<std::size_t>(std::clamp(stepSize, std::uint64_t{1}, maximumStepSize));
    }
    item_tooltip(fmt::format("Entries per step. Increase: {}; decrease: {}.",
                             state.increaseStepKeysLabel,
                             state.decreaseStepKeysLabel));

    ImGui::Spacing();
    const bool canSkipToEnd = state.currentEntry < state.totalEntries;
    if (!canSkipToEnd) {
        ImGui::BeginDisabled();
    }
    if (full_width_button("Skip to end")) {
        state.command = ReplayGuiState::Command::SkipToEnd;
    }
    item_tooltip("Apply all remaining entries and stay in paused replay mode.");
    if (!canSkipToEnd) {
        ImGui::EndDisabled();
    }
    if (full_width_button("End replay")) {
        state.command = ReplayGuiState::Command::EndReplay;
    }
    item_tooltip("Apply all remaining entries, leave replay mode, then process queued live messages.");
}

void render_release_display_controls(renderer::Renderer& renderer) {
    if (!ImGui::CollapsingHeader("Display")) {
        return;
    }

    const int currentSamples = renderer.get_msaa_samples();
    const int maximumSamples = renderer.get_max_msaa_samples();
    const std::string preview = currentSamples == 1 ? "Off (1x)" : fmt::format("{}x", currentSamples);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("MSAA");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0F);
    if (ImGui::BeginCombo("##MSAA", preview.c_str())) {
        for (int samples = 1; samples <= maximumSamples;) {
            const bool selected = samples == currentSamples;
            const std::string label = samples == 1 ? "MSAA: Off (1x)" : fmt::format("MSAA: {}x", samples);
            if (ImGui::Selectable(label.c_str(), selected)) {
                renderer.set_msaa_samples(samples);
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
            if (samples > maximumSamples / 2) {
                break;
            }
            samples *= 2;
        }
        ImGui::EndCombo();
    }
    item_tooltip("Multisample anti-aliasing.");

    bool fxaaEnabled = renderer.get_fxaa_enabled();
    if (ImGui::Checkbox("FXAA", &fxaaEnabled)) {
        renderer.set_fxaa_enabled(fxaaEnabled);
    }
    float exposureStops = renderer.get_exposure_stops();
    if (ImGui::SliderFloat("Exposure", &exposureStops, exposureMinimum, exposureMaximum, "%.1f stops")) {
        renderer.set_exposure_stops(exposureStops);
    }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void render_debug_display_controls(renderer::Renderer& renderer) {
    if (ImGui::CollapsingHeader("Visualization", ImGuiTreeNodeFlags_DefaultOpen)) {
        constexpr std::array<const char*, 10> modes{"Final",
                                                    "Raw HDR",
                                                    "Linear LDR (unencoded)",
                                                    "Luminance",
                                                    "Log Luminance",
                                                    "Depth",
                                                    "Overexposure",
                                                    "Underexposure",
                                                    "NaN & Infinity",
                                                    "Grayscale"};
        int selectedMode = static_cast<int>(renderer.get_visualization_mode());
        if (ImGui::Combo("Mode", &selectedMode, modes.data(), static_cast<int>(modes.size()))) {
            renderer.set_visualization_mode(static_cast<renderer::VisualizationMode>(selectedMode));
        }

        const renderer::VisualizationMode mode = renderer.get_visualization_mode();
        const bool toneMappingApplies = mode == renderer::VisualizationMode::Final ||
                                        mode == renderer::VisualizationMode::LinearLdr ||
                                        mode == renderer::VisualizationMode::Grayscale;
        if (toneMappingApplies) {
            constexpr std::array<const char*, 2> toneMappingModes{"None (clamp)", "Reinhard"};
            int toneMappingMode = static_cast<int>(renderer.get_tone_map_mode());
            if (ImGui::Combo("Tone mapping",
                             &toneMappingMode,
                             toneMappingModes.data(),
                             static_cast<int>(toneMappingModes.size()))) {
                renderer.set_tone_map_mode(static_cast<renderer::ToneMapMode>(toneMappingMode));
            }
        }
        if (mode == renderer::VisualizationMode::RawHdr || mode == renderer::VisualizationMode::Luminance) {
            float hdrMaximum = renderer.get_hdr_display_max();
            if (ImGui::SliderFloat("HDR maximum", &hdrMaximum, hdrDisplayMinimum, hdrDisplayMaximum, "%.1f")) {
                renderer.set_hdr_display_max(hdrMaximum);
            }
        }
        if (mode == renderer::VisualizationMode::Final) {
            bool grayscale = renderer.get_grayscale();
            if (ImGui::Checkbox("Grayscale", &grayscale)) {
                renderer.set_grayscale(grayscale);
            }
        }
    }

    render_release_display_controls(renderer);
    if (renderer.get_fxaa_enabled() && ImGui::CollapsingHeader("FXAA tuning")) {
        float edgeThreshold = renderer.get_fxaa_edge_threshold();
        if (ImGui::SliderFloat("Edge threshold", &edgeThreshold, 0.0F, fxaaEdgeThresholdMaximum, "%.3f")) {
            renderer.set_fxaa_edge_threshold(edgeThreshold);
        }
        float minimumEdgeContrast = renderer.get_fxaa_edge_threshold_min();
        if (ImGui::SliderFloat("Minimum edge contrast", &minimumEdgeContrast, 0.0F, fxaaMinimumEdgeContrastMaximum, "%.4f")) {
            renderer.set_fxaa_edge_threshold_min(minimumEdgeContrast);
        }
        float subpixelAmount = renderer.get_fxaa_subpixel_amount();
        if (ImGui::SliderFloat("Subpixel amount", &subpixelAmount, 0.0F, 1.0F, "%.2f")) {
            renderer.set_fxaa_subpixel_amount(subpixelAmount);
        }
    }
}

void render_file_error_popup(FileGuiState& state) {
    if (state.openErrorPopup) {
        ImGui::OpenPopup("File operation failed###GeoQikFileError");
        state.openErrorPopup = false;
    }
    if (ImGui::BeginPopupModal("File operation failed###GeoQikFileError", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        ImGui::TextWrapped("%s", state.errorMessage.c_str());
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2{errorPopupButtonWidth, 0.0F})) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

float panel_max_width(float viewportWidth) {
    return std::max(1.0F, std::min(controlPanelMaxWidth, viewportWidth));
}

float panel_min_width(float viewportWidth) {
    return std::min(controlPanelMinWidth, panel_max_width(viewportWidth));
}

float clamp_panel_width(float width, float viewportWidth) {
    return std::clamp(width, panel_min_width(viewportWidth), panel_max_width(viewportWidth));
}

void render_panel_resize_grip(float& panelWidth, float viewportWidth, float height) {
    ImGui::InvisibleButton("##GeoQikPanelResizeGrip",
                           ImVec2{resizeGripWidth, height},
                           ImGuiButtonFlags_MouseButtonLeft);
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    if (hovered || active) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    if (active) {
        panelWidth = clamp_panel_width(panelWidth + ImGui::GetIO().MouseDelta.x, viewportWidth);
    }

    ImGuiCol dividerColor = ImGuiCol_Separator;
    if (active) {
        dividerColor = ImGuiCol_SeparatorActive;
    } else if (hovered) {
        dividerColor = ImGuiCol_SeparatorHovered;
    }
    const ImVec2 gripMin = ImGui::GetItemRectMin();
    const ImVec2 gripMax = ImGui::GetItemRectMax();
    const float dividerX = gripMax.x - 1.0F;
    ImGui::GetWindowDrawList()->AddLine(ImVec2{dividerX, gripMin.y},
                                        ImVec2{dividerX, gripMax.y},
                                        ImGui::GetColorU32(dividerColor),
                                        1.0F);
}

} // namespace

GeoQikOverlay::GeoQikOverlay(void* nativeWindow)
    : m_inner(nativeWindow) {
    m_fileState.nativeDialogsInitialized = NFD_Init() == NFD_OKAY;
    if (!m_fileState.nativeDialogsInitialized) {
        const char* detail = NFD_GetError();
        fmt::print("Could not initialize native file dialogs{}{}\n",
                   detail == nullptr ? "" : ": ",
                   detail == nullptr ? "" : detail);
    }

    m_fileState.settingsFilePath = user_settings_file_path();
    try {
        m_fileState.defaultLogDirectory = load_user_settings(m_fileState.settingsFilePath).defaultLogDirectory;
    } catch (const std::exception& exception) {
        m_fileState.defaultLogDirectory = default_log_directory();
        fmt::print("Could not load GeoQik user settings: {}\n", exception.what());
    }
}

GeoQikOverlay::~GeoQikOverlay() {
    shutdown_native_dialogs();
}

void GeoQikOverlay::shutdown_native_dialogs() {
    if (m_fileState.nativeDialogsInitialized) {
        NFD_Quit();
        m_fileState.nativeDialogsInitialized = false;
    }
}

void GeoQikOverlay::add_replay_controls() {
    m_controls.emplace_back([this]() { render_replay_controls(m_replayState); });
}

void GeoQikOverlay::add_camera_controls() {
    m_controls.emplace_back([this]() { render_camera_controls(m_cameraState, m_replayState.isActive); });
}

void GeoQikOverlay::add_display_controls(renderer::Renderer& renderer) {
    if (m_uiMode == renderer::UiMode::Debug) {
        m_controls.emplace_back([renderer = &renderer]() { render_debug_display_controls(*renderer); });
    } else {
        m_controls.emplace_back([renderer = &renderer]() { render_release_display_controls(*renderer); });
    }
}

void GeoQikOverlay::apply_scene_viewport_hint(renderer::OverlayFrameContext& context) const {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (viewport == nullptr) {
        return;
    }
    const float sceneWidth = std::max(1.0F, viewport->WorkSize.x - m_controlPanelWidth);
    context.sceneViewportHint =
        renderer::LogicalViewportRect{static_cast<double>(viewport->WorkPos.x + m_controlPanelWidth),
                                      static_cast<double>(viewport->WorkPos.y),
                                      static_cast<double>(sceneWidth),
                                      static_cast<double>(viewport->WorkSize.y)};
}

void GeoQikOverlay::build_controls(renderer::OverlayFrameContext& context) {
    render_main_menu_bar();
    render_recording_badge();
    render_recording_status();
    apply_scene_viewport_hint(context);

    m_cameraState.projectionType = context.projectionType;
    if (m_replayState.isActive) {
        add_replay_controls();
    }
    add_camera_controls();
    add_display_controls(context.renderer);
}

void GeoQikOverlay::render_recording_badge() {
    if (!m_videoState.isRecording) {
        return;
    }

    // Draw an always-visible "REC" badge in the top-right of the scene area so the user can tell a
    // recording is running even with all menus closed. The badge text/timer is drawn via ImGui's
    // foreground draw list and is not part of the recorded pixels (capture reads the swapped front
    // buffer). A borderless overlay window pins a clickable Stop button under the badge so the user
    // can end a recording without reopening the menu.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (viewport == nullptr) {
        return;
    }

    const int seconds = static_cast<int>(m_videoState.elapsedSeconds);
    const std::string label = fmt::format("\xE2\x97\x8F REC  {:02d}:{:02d}  {}x{}  {}fps",
                                          seconds / 60,
                                          seconds % 60,
                                          m_videoState.width,
                                          m_videoState.height,
                                          m_videoState.fps);

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
    const ImVec2 topRight{viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y};
    const ImVec2 boxMin{topRight.x - textSize.x - 2.0F * recordingBadgePadding - recordingBadgeMargin,
                        topRight.y + recordingBadgeMargin};
    const ImVec2 boxMax{topRight.x - recordingBadgeMargin, boxMin.y + textSize.y + 2.0F * recordingBadgePadding};
    drawList->AddRectFilled(boxMin, boxMax, IM_COL32(0, 0, 0, 160), recordingBadgeRounding);
    drawList->AddText(ImVec2{boxMin.x + recordingBadgePadding, boxMin.y + recordingBadgePadding},
                      IM_COL32(255, 64, 64, 255), label.c_str());

    // A borderless, no-nav overlay window holds the actual Stop hit-target directly beneath the
    // badge. It lives in the overlay (like the badge) so it is not captured into the recording.
    ImGui::SetNextWindowPos(ImVec2{boxMin.x, boxMax.y + recordingBadgeMargin});
    ImGui::SetNextWindowBgAlpha(0.0F);
    constexpr ImGuiWindowFlags overlayFlags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("##GeoQikRecStop", nullptr, overlayFlags)) {
        if (ImGui::Button("\xE2\x96\xA0 Stop")) {
            m_videoState.command = VideoGuiState::Command::StopRecording;
        }
        item_tooltip("Stop the current recording.");
    }
    ImGui::End();
}

void GeoQikOverlay::render_recording_status() {
    VideoGuiState& state = m_videoState;
    if (state.statusKind == VideoGuiState::StatusKind::None) {
        return;
    }

    const double now = ImGui::GetTime();
    // Context has no ImGui context, so it flags a freshly-set message; stamp the display time here.
    if (state.statusIsNew) {
        state.statusSetAtSeconds = now;
        state.statusIsNew = false;
    }

    // A recording in progress keeps its "Recording…" info line up until stop replaces it; other
    // messages fade after a few seconds so they do not linger.
    const bool persistent = state.isRecording && state.statusKind == VideoGuiState::StatusKind::Info;
    if (!persistent && now - state.statusSetAtSeconds > recordingStatusTimeoutSeconds) {
        state.statusKind = VideoGuiState::StatusKind::None;
        state.statusMessage.clear();
        return;
    }

    // Transient toast near the bottom-center of the scene so success/failure is visible with all
    // menus closed. Drawn in the overlay, so it is not captured into the recording.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (viewport == nullptr) {
        return;
    }
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImVec2 textSize = ImGui::CalcTextSize(state.statusMessage.c_str());
    const ImVec2 center{viewport->WorkPos.x + viewport->WorkSize.x * 0.5F,
                        viewport->WorkPos.y + viewport->WorkSize.y - recordingBadgeMargin * 4.0F};
    const ImVec2 boxMin{center.x - textSize.x * 0.5F - recordingBadgePadding, center.y - recordingBadgePadding};
    const ImVec2 boxMax{center.x + textSize.x * 0.5F + recordingBadgePadding,
                        center.y + textSize.y + recordingBadgePadding};
    drawList->AddRectFilled(boxMin, boxMax, IM_COL32(0, 0, 0, 180), recordingBadgeRounding);
    drawList->AddText(ImVec2{boxMin.x + recordingBadgePadding, boxMin.y + recordingBadgePadding},
                      recording_status_color(state.statusKind), state.statusMessage.c_str());
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void GeoQikOverlay::render_main_menu_bar() {
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }

    if (ImGui::BeginMenu("File")) {
        if (ImGui::BeginMenu("Save Log", m_fileState.nativeDialogsInitialized)) {
            if (ImGui::MenuItem("Binary (.gqklog)...")) {
                m_fileState.dialogRequest = FileGuiState::DialogRequest::SaveBinary;
            }
            item_tooltip("Compact GeoQik binary format.");
            if (ImGui::MenuItem("JSON (.json)...")) {
                m_fileState.dialogRequest = FileGuiState::DialogRequest::SaveJson;
            }
            item_tooltip("Human-readable JSON format.");
            ImGui::EndMenu();
        } else if (!m_fileState.nativeDialogsInitialized) {
            item_tooltip("Native file dialogs could not be initialized.");
        }

        if (ImGui::MenuItem("Load Log...", nullptr, false, m_fileState.nativeDialogsInitialized)) {
            m_fileState.dialogRequest = FileGuiState::DialogRequest::Load;
        }
        item_tooltip(m_fileState.nativeDialogsInitialized ? "Load a .gqklog or .json log."
                                                          : "Native file dialogs could not be initialized.");
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Replay")) {
        if (ImGui::MenuItem("Load Log...", nullptr, false, m_fileState.nativeDialogsInitialized)) {
            m_fileState.dialogRequest = FileGuiState::DialogRequest::Replay;
        }
        item_tooltip(m_fileState.nativeDialogsInitialized ? "Load a .gqklog or .json log and start replaying it."
                                                          : "Native file dialogs could not be initialized.");
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Settings")) {
        if (ImGui::MenuItem("Default Log Directory...", nullptr, false, m_fileState.nativeDialogsInitialized)) {
            m_fileState.dialogRequest = FileGuiState::DialogRequest::SelectDefaultDirectory;
        }
        item_tooltip(m_fileState.nativeDialogsInitialized
                         ? fmt::format("Current: {}", path_to_utf8(m_fileState.defaultLogDirectory))
                         : "Native file dialogs could not be initialized.");
        ImGui::EndMenu();
    }

    render_record_menu();

    ImGui::EndMainMenuBar();
    switch (std::exchange(m_fileState.dialogRequest, FileGuiState::DialogRequest::None)) {
    case FileGuiState::DialogRequest::SaveBinary:             select_save_log_path(m_fileState, GEOQIK_LOG_FORMAT_BINARY); break;
    case FileGuiState::DialogRequest::SaveJson:               select_save_log_path(m_fileState, GEOQIK_LOG_FORMAT_JSON); break;
    case FileGuiState::DialogRequest::Load:                   select_log_file(m_fileState, FileGuiState::Command::Load); break;
    case FileGuiState::DialogRequest::Replay:                 select_log_file(m_fileState, FileGuiState::Command::Replay); break;
    case FileGuiState::DialogRequest::SelectDefaultDirectory: select_default_log_directory(m_fileState); break;
    case FileGuiState::DialogRequest::None:                   break;
    }
    switch (std::exchange(m_videoState.dialogRequest, VideoGuiState::DialogRequest::None)) {
    case VideoGuiState::DialogRequest::SelectFfmpeg:             select_ffmpeg_executable(m_videoState, m_fileState); break;
    case VideoGuiState::DialogRequest::SelectRecordingDirectory: select_recording_directory(m_videoState, m_fileState); break;
    case VideoGuiState::DialogRequest::SelectLogForVideo:        select_log_for_video(m_videoState, m_fileState); break;
    case VideoGuiState::DialogRequest::None:                     break;
    }
    render_file_error_popup(m_fileState);
}

std::pair<int, int> video_resolution_preset(int index) {
    if (index < 0 || index >= static_cast<int>(videoResolutionPresets.size())) {
        return {0, 0};
    }
    const ResolutionPreset& preset = videoResolutionPresets[static_cast<std::size_t>(index)];
    return {preset.width, preset.height};
}

void GeoQikOverlay::render_log_video_settings(VideoGuiState& state) {
    constexpr float fieldWidth = logVideoFieldWidth;

    // Output format for the rendered log.
    constexpr std::array<const char*, 3> formatLabels{"MP4 (H.264)", "WebM (VP9)", "GIF"};
    constexpr std::array<VideoGuiState::Format, 3> formatValues{
        VideoGuiState::Format::Mp4, VideoGuiState::Format::WebM, VideoGuiState::Format::Gif};
    const auto formatIt = std::ranges::find(formatValues, state.requestedFormat);
    int formatIndex =
        formatIt == formatValues.end() ? 0 : static_cast<int>(std::distance(formatValues.begin(), formatIt));
    ImGui::TextUnformatted("Format");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(fieldWidth);
    if (ImGui::Combo("##LogVideoFormat", &formatIndex, formatLabels.data(), static_cast<int>(formatLabels.size()))) {
        state.requestedFormat = formatValues[static_cast<std::size_t>(formatIndex)];
    }

    // Resolution preset.
    std::array<const char*, videoResolutionPresets.size()> resolutionLabels{};
    for (std::size_t i = 0; i < videoResolutionPresets.size(); ++i) {
        resolutionLabels[i] = videoResolutionPresets[i].label;
    }
    ImGui::TextUnformatted("Resolution");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(fieldWidth);
    ImGui::Combo("##LogVideoResolution", &state.resolutionPresetIndex, resolutionLabels.data(),
                 static_cast<int>(resolutionLabels.size()));
    item_tooltip("Non-window sizes briefly resize the window while rendering.");

    // Frames per second.
    ImGui::TextUnformatted("FPS");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(fieldWidth);
    ImGui::SliderInt("##LogVideoFps", &state.requestedFps, logVideoMinFps, logVideoMaxFps);

    ImGui::Separator();
    ImGui::TextUnformatted("Pacing");

    // Speed vs. Duration are mutually exclusive: the inactive field is disabled.
    bool speedSelected = state.pacing == VideoGuiState::Pacing::Speed;
    if (ImGui::RadioButton("Speed", speedSelected)) {
        state.pacing = VideoGuiState::Pacing::Speed;
        speedSelected = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Duration", !speedSelected)) {
        state.pacing = VideoGuiState::Pacing::Duration;
        speedSelected = false;
    }

    ImGui::BeginDisabled(!speedSelected);
    ImGui::SetNextItemWidth(fieldWidth);
    ImGui::InputFloat("entries / second", &state.entriesPerSecond, 1.0F, 10.0F, "%.1f");
    ImGui::EndDisabled();
    item_tooltip("Speed mode: how many log entries play per second. Duration = entries / this.");

    ImGui::BeginDisabled(speedSelected);
    ImGui::SetNextItemWidth(fieldWidth);
    ImGui::InputFloat("target duration (s)", &state.targetDurationSeconds, 0.5F, 5.0F, "%.1f");
    ImGui::EndDisabled();
    item_tooltip("Duration mode: total video length for the log body, regardless of entry count.");

    ImGui::Separator();
    ImGui::SetNextItemWidth(fieldWidth);
    ImGui::InputFloat("hold start (s)", &state.holdStartSeconds, 0.25F, 1.0F, "%.2f");
    ImGui::SetNextItemWidth(fieldWidth);
    ImGui::InputFloat("hold end (s)", &state.holdEndSeconds, 0.25F, 1.0F, "%.2f");
    item_tooltip("Freeze the first/last frame for this many seconds.");

    // Clamp negatives that InputFloat's step buttons could produce.
    state.entriesPerSecond = std::max(0.1F, state.entriesPerSecond);
    state.targetDurationSeconds = std::max(0.1F, state.targetDurationSeconds);
    state.holdStartSeconds = std::max(0.0F, state.holdStartSeconds);
    state.holdEndSeconds = std::max(0.0F, state.holdEndSeconds);

    ImGui::Separator();
    if (ImGui::Button("Choose Log and Render...")) {
        // Opens the log picker; the picker sets RenderLogToVideo, and consume_video_gui_commands
        // reads the settings gathered above.
        state.dialogRequest = VideoGuiState::DialogRequest::SelectLogForVideo;
        ImGui::CloseCurrentPopup();
    }
}

void GeoQikOverlay::render_record_menu() {
    VideoGuiState& state = m_videoState;
    const bool dialogsReady = m_fileState.nativeDialogsInitialized;
    const char* menuLabel = state.isRecording ? "Record \xE2\x97\x8F" : "Record";
    if (ImGui::BeginMenu(menuLabel)) {
        if (state.isRecording) {
            if (ImGui::MenuItem("\xE2\x96\xA0 Stop Recording")) {
                state.command = VideoGuiState::Command::StopRecording;
            }
            item_tooltip(fmt::format("Recording {}x{} @ {} fps ({:.0f}s)",
                                     state.width,
                                     state.height,
                                     state.fps,
                                     state.elapsedSeconds));
        } else {
            const bool canRecordVideo = state.ffmpegAvailable && dialogsReady;
            if (ImGui::MenuItem("Start Recording (MP4)", nullptr, false, canRecordVideo)) {
                state.requestedFormat = VideoGuiState::Format::Mp4;
                state.command = VideoGuiState::Command::StartRecording;
            }
            item_tooltip(state.ffmpegAvailable ? "Record the live session to an H.264 MP4."
                                               : "Set the ffmpeg.exe path to enable video recording.");
            if (ImGui::MenuItem("Start Recording (WebM)", nullptr, false, canRecordVideo)) {
                state.requestedFormat = VideoGuiState::Format::WebM;
                state.command = VideoGuiState::Command::StartRecording;
            }
            item_tooltip(state.ffmpegAvailable ? "Record the live session to a VP9 WebM."
                                               : "Set the ffmpeg.exe path to enable video recording.");
            if (ImGui::MenuItem("Start Recording (PNG sequence)", nullptr, false, dialogsReady)) {
                state.requestedFormat = VideoGuiState::Format::PngSequence;
                state.command = VideoGuiState::Command::StartRecording;
            }
            item_tooltip("Write numbered PNG frames. Requires no ffmpeg.");

            // Make the reason MP4/WebM are greyed out visible without hovering each disabled item.
            if (!state.ffmpegAvailable) {
                ImGui::TextDisabled("ffmpeg not set \xE2\x80\x94 PNG sequence still works");
            }

            ImGui::Separator();
            if (ImGui::BeginMenu("Render Log to Video...", canRecordVideo)) {
                render_log_video_settings(state);
                ImGui::EndMenu();
            }
            item_tooltip(state.ffmpegAvailable ? "Configure and render a log deterministically to a video."
                                               : "Set the ffmpeg.exe path to enable video recording.");
        }

        ImGui::Separator();
        // Quality applies to both live recording and log->video.
        {
            constexpr std::array<const char*, 4> qualityLabels{"High (sharp)", "Medium", "Low", "Lossless"};
            int qualityIndex = static_cast<int>(state.quality);
            ImGui::TextUnformatted("Quality");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(recordQualityFieldWidth);
            if (ImGui::Combo("##RecordQuality", &qualityIndex, qualityLabels.data(),
                             static_cast<int>(qualityLabels.size()))) {
                state.quality = static_cast<VideoGuiState::Quality>(qualityIndex);
            }
            item_tooltip("Higher quality = sharper edges and larger files.");
        }

        ImGui::Separator();
        ImGui::TextDisabled("Settings");
        if (ImGui::MenuItem("Recording Folder...", nullptr, false, dialogsReady)) {
            state.dialogRequest = VideoGuiState::DialogRequest::SelectRecordingDirectory;
        }
        item_tooltip(fmt::format("Current: {}", path_to_utf8(state.recordingDirectory)));
        if (ImGui::MenuItem("Set ffmpeg.exe Path...", nullptr, false, dialogsReady)) {
            state.dialogRequest = VideoGuiState::DialogRequest::SelectFfmpeg;
        }
        item_tooltip(state.ffmpegAvailable ? fmt::format("Current: {}", path_to_utf8(state.ffmpegPath))
                                           : "ffmpeg not found. Click to choose ffmpeg.exe.");

        ImGui::Separator();
        const bool haveOutput = !state.lastOutputPath.empty();
        if (ImGui::MenuItem("Reveal Last Recording", nullptr, false, haveOutput)) {
            state.command = VideoGuiState::Command::RevealLastOutput;
        }
        if (haveOutput) {
            item_tooltip(fmt::format("Saved to: {}", path_to_utf8(state.lastOutputPath)));
        }
        if (ImGui::MenuItem("Open Recordings Folder")) {
            state.command = VideoGuiState::Command::OpenRecordingDirectory;
        }

        // Inline, non-blocking status for the last recording action (mirrors the badge toast).
        if (state.statusKind != VideoGuiState::StatusKind::None) {
            ImGui::Separator();
            ImGui::PushTextWrapPos(recordQualityFieldWidth + logVideoFieldWidth);
            ImGui::PushStyleColor(ImGuiCol_Text, recording_status_color(state.statusKind));
            ImGui::TextUnformatted(state.statusMessage.c_str());
            ImGui::PopStyleColor();
            ImGui::PopTextWrapPos();
        }
        ImGui::EndMenu();
    }
}

void GeoQikOverlay::layout_controls() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (viewport == nullptr) {
        m_controls.clear();
        return;
    }

    m_controlPanelWidth = clamp_panel_width(m_controlPanelWidth, viewport->WorkSize.x);
    ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2{m_controlPanelWidth, viewport->WorkSize.y}, ImGuiCond_Always);

    constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
                                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                                             ImGuiWindowFlags_NoSavedSettings;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
    if (ImGui::Begin("GeoQik Controls", nullptr, windowFlags)) {
        const ImVec2 availableContentSize = ImGui::GetContentRegionAvail();
        const float contentWidth = std::max(1.0F, availableContentSize.x - resizeGripWidth);
        if (ImGui::BeginChild("##GeoQikPanelContent", ImVec2{contentWidth, 0.0F}, 0)) {
            int id = 0;
            for (const auto& control: m_controls) {
                ImGui::PushID(id++);
                control();
                ImGui::PopID();
            }
        }
        ImGui::EndChild();

        ImGui::SameLine(0.0F, 0.0F);
        render_panel_resize_grip(m_controlPanelWidth, viewport->WorkSize.x, availableContentSize.y);
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
    m_controls.clear();
}

void GeoQikOverlay::render() {
    layout_controls();
    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    if (drawData != nullptr && drawData->Valid) {
        ImGui_ImplOpenGL3_RenderDrawData(drawData);
    }
}

} // namespace geoqik
