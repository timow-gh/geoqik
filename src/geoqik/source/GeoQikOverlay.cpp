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

std::string path_to_utf8(const std::filesystem::path& path) {
    const std::u8string value = path.u8string();
    return {reinterpret_cast<const char*>(value.data()), value.size()};
}

std::filesystem::path path_from_utf8(const char* value) {
    const std::string_view bytes{value};
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
        ImGui::PushTextWrapPos(360.0F);
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

void select_log_to_load(FileGuiState& state) {
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
        state.command = FileGuiState::Command::Load;
    } else if (result == NFD_ERROR) {
        set_file_dialog_error(state, "Could not open the load dialog");
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
        ImGui::TableSetupColumn("##AutoFitColumn", ImGuiTableColumnFlags_WidthStretch, 0.8F);
        ImGui::TableSetupColumn("##ProjectionColumn", ImGuiTableColumnFlags_WidthStretch, 1.2F);
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
    const float buttonWidth = equal_button_width(static_cast<int>(speedOptions.size()));
    for (std::size_t index = 0; index < speedOptions.size(); ++index) {
        if (index != 0) {
            ImGui::SameLine();
        }
        const bool current = std::abs(state.speedMultiplier - speedOptions[index]) < 0.01;
        if (highlighted_button(speedLabels[index], buttonWidth, current)) {
            state.requestedSpeedMultiplier = speedOptions[index];
        }
    }
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

    const char* status = state.isPaused ? "Paused" : (state.isBackward ? "Reversing" : "Playing");
    ImGui::TextUnformatted(fmt::format("{}  -  {} / {}", status, state.currentEntry, state.totalEntries).c_str());

    std::uint64_t position = static_cast<std::uint64_t>(state.currentEntry);
    constexpr std::uint64_t firstEntry = 0;
    const std::uint64_t lastEntry = static_cast<std::uint64_t>(state.totalEntries);
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
    if (ImGui::SliderFloat("Exposure", &exposureStops, -10.0F, 10.0F, "%.1f stops")) {
        renderer.set_exposure_stops(exposureStops);
    }
}

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
            if (ImGui::SliderFloat("HDR maximum", &hdrMaximum, 0.1F, 100.0F, "%.1f")) {
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
        if (ImGui::SliderFloat("Edge threshold", &edgeThreshold, 0.0F, 0.5F, "%.3f")) {
            renderer.set_fxaa_edge_threshold(edgeThreshold);
        }
        float minimumEdgeContrast = renderer.get_fxaa_edge_threshold_min();
        if (ImGui::SliderFloat("Minimum edge contrast", &minimumEdgeContrast, 0.0F, 0.25F, "%.4f")) {
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
        ImGui::TextWrapped("%s", state.errorMessage.c_str());
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2{120.0F, 0.0F})) {
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
    apply_scene_viewport_hint(context);

    m_cameraState.projectionType = context.projectionType;
    if (m_replayState.isActive) {
        add_replay_controls();
    }
    add_camera_controls();
    add_display_controls(context.renderer);
}

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

    if (ImGui::BeginMenu("Settings")) {
        if (ImGui::MenuItem("Default Log Directory...", nullptr, false, m_fileState.nativeDialogsInitialized)) {
            m_fileState.dialogRequest = FileGuiState::DialogRequest::SelectDefaultDirectory;
        }
        item_tooltip(m_fileState.nativeDialogsInitialized
                         ? fmt::format("Current: {}", path_to_utf8(m_fileState.defaultLogDirectory))
                         : "Native file dialogs could not be initialized.");
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
    switch (std::exchange(m_fileState.dialogRequest, FileGuiState::DialogRequest::None)) {
    case FileGuiState::DialogRequest::SaveBinary:             select_save_log_path(m_fileState, GEOQIK_LOG_FORMAT_BINARY); break;
    case FileGuiState::DialogRequest::SaveJson:               select_save_log_path(m_fileState, GEOQIK_LOG_FORMAT_JSON); break;
    case FileGuiState::DialogRequest::Load:                   select_log_to_load(m_fileState); break;
    case FileGuiState::DialogRequest::SelectDefaultDirectory: select_default_log_directory(m_fileState); break;
    case FileGuiState::DialogRequest::None:                   break;
    }
    render_file_error_popup(m_fileState);
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
