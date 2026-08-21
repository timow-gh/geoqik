#include "Video/FfmpegLocator.hpp"

#include <boost/process/v1/child.hpp>
#include <boost/process/v1/io.hpp>
#include <boost/process/v1/search_path.hpp>

#include <array>
#include <cstdlib>
#include <system_error>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace bp = boost::process::v1;

namespace geoqik::video {

namespace {

#ifdef _WIN32
constexpr const char* kFfmpegExecutableName = "ffmpeg.exe";
#else
constexpr const char* kFfmpegExecutableName = "ffmpeg";
#endif

[[nodiscard]] std::filesystem::path executable_directory() {
#ifdef _WIN32
    std::vector<wchar_t> buffer(1024);
    for (;;) {
        const DWORD length = ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return {};
        }
        if (length < buffer.size()) {
            return std::filesystem::path{std::wstring{buffer.data(), length}}.parent_path();
        }
        buffer.resize(buffer.size() * 2); // Path was truncated; grow and retry.
    }
#else
    std::error_code error;
    const std::filesystem::path exe = std::filesystem::read_symlink("/proc/self/exe", error);
    return error ? std::filesystem::path{} : exe.parent_path();
#endif
}

#ifdef _WIN32
[[nodiscard]] std::filesystem::path env_path(const wchar_t* name) {
    wchar_t* value = nullptr;
    std::size_t length = 0;
    if (::_wdupenv_s(&value, &length, name) != 0 || value == nullptr) {
        return {};
    }
    const std::filesystem::path path = value[0] == L'\0' ? std::filesystem::path{} : std::filesystem::path{value};
    std::free(value);
    return path;
}
#else
[[nodiscard]] std::filesystem::path env_path(const char* name) {
    const char* value = std::getenv(name);
    return value == nullptr || value[0] == '\0' ? std::filesystem::path{} : std::filesystem::path{value};
}
#endif

} // namespace

bool FfmpegLocator::validate(const std::filesystem::path& ffmpegPath) {
    if (ffmpegPath.empty()) {
        return false;
    }
    std::error_code error;
    if (!std::filesystem::exists(ffmpegPath, error) && ffmpegPath.has_parent_path()) {
        // A bare "ffmpeg" (resolved via PATH later) has no parent; only reject explicit paths.
        return false;
    }

    try {
        // Suppress ffmpeg's output; we only care about the exit code.
        bp::child child(ffmpegPath.string(),
                        "-version",
                        bp::std_out > bp::null,
                        bp::std_err > bp::null,
                        bp::std_in < bp::null,
                        error);
        if (error) {
            return false;
        }
        child.wait(error);
        if (error) {
            return false;
        }
        return child.exit_code() == 0;
    } catch (...) {
        return false;
    }
}

std::optional<std::filesystem::path> FfmpegLocator::find(const std::filesystem::path& userConfiguredPath) {
    // 1. Explicit, user-configured path.
    if (!userConfiguredPath.empty() && validate(userConfiguredPath)) {
        return userConfiguredPath;
    }

    // 2. PATH lookup. boost::process returns a boost::filesystem::path; convert via its string.
    std::error_code error;
    const boost::filesystem::path onPathBoost = bp::search_path("ffmpeg");
    if (!onPathBoost.empty()) {
        const std::filesystem::path onPath{onPathBoost.string()};
        if (validate(onPath)) {
            return onPath;
        }
    }

    // 3. Next to the application executable (supports bundling ffmpeg alongside geoqik).
    const std::filesystem::path exeDir = executable_directory();
    if (!exeDir.empty()) {
        const std::filesystem::path sibling = exeDir / kFfmpegExecutableName;
        if (std::filesystem::exists(sibling, error) && validate(sibling)) {
            return sibling;
        }
    }

    // 4. Common install locations.
    std::vector<std::filesystem::path> candidates;
#ifdef _WIN32
    for (const wchar_t* var : {L"ProgramFiles", L"ProgramFiles(x86)", L"LOCALAPPDATA"}) {
        const std::filesystem::path base = env_path(var);
        if (!base.empty()) {
            candidates.push_back(base / "ffmpeg" / "bin" / kFfmpegExecutableName);
        }
    }
    // Scoop shim directory.
    const std::filesystem::path userProfile = env_path(L"USERPROFILE");
    if (!userProfile.empty()) {
        candidates.push_back(userProfile / "scoop" / "shims" / kFfmpegExecutableName);
    }
#else
    for (const char* dir : {"/usr/bin", "/usr/local/bin", "/opt/homebrew/bin"}) {
        candidates.push_back(std::filesystem::path{dir} / kFfmpegExecutableName);
    }
#endif

    for (const std::filesystem::path& candidate : candidates) {
        if (std::filesystem::exists(candidate, error) && validate(candidate)) {
            return candidate;
        }
    }

    return std::nullopt;
}

} // namespace geoqik::video
