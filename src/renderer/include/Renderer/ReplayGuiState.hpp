#ifndef RENDERER_REPLAYGUISTATE_HPP
#define RENDERER_REPLAYGUISTATE_HPP

#include <cstddef>
#include <string>

namespace renderer
{

struct ReplayGuiState
{
  // --- Status: filled by Context before calling end_frame ---
  bool     isActive{false};
  bool     isPaused{false};
  bool     isBackward{false};
  std::size_t currentEntry{0};
  std::size_t totalEntries{0};
  double   speedMultiplier{1.0};
  std::size_t entriesPerStep{1};
  std::string pauseKeysLabel;
  std::string resumeKeysLabel;
  std::string stepForwardKeysLabel;
  std::string stepBackwardKeysLabel;
  std::string increaseStepKeysLabel;
  std::string decreaseStepKeysLabel;

  // --- Commands: filled by ImGui, consumed by Context ---
  enum class Command
  {
    None,
    Play,
    PlayReverse,
    Pause,
    Finish,
    StepForward,
    StepBackward,
  };
  Command  command{Command::None};
  double   requestedSpeedMultiplier{0.0};
  std::size_t requestedEntriesPerStep{0};
};

} // namespace renderer

#endif // RENDERER_REPLAYGUISTATE_HPP
