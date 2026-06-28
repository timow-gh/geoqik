#ifndef GEOQIK_CLIENT_DETAIL_PROCESS_MANAGER_HPP
#define GEOQIK_CLIENT_DETAIL_PROCESS_MANAGER_HPP

#include <GeoQikClient/detail/ClientTypes.hpp>
#include <GeoQikClient/detail/Protocol.hpp>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#else
#  include <cerrno>
#  include <cstring>
#  include <sys/types.h>
#  include <sys/wait.h>
#  include <unistd.h>
#endif

namespace geoqik::client::detail {

class ServerExecutableNotFoundError : public std::runtime_error {
public:
    explicit ServerExecutableNotFoundError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class ServerStartError : public std::runtime_error {
public:
    explicit ServerStartError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class ProcessManager {
public:
    [[nodiscard]] static ProcessManager& instance() {
        static ProcessManager pm;
        return pm;
    }

    ProcessManager(const ProcessManager&) = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;
    ProcessManager(ProcessManager&&) = delete;
    ProcessManager& operator=(ProcessManager&&) = delete;

    ~ProcessManager() {
#ifdef _WIN32
        close_process_handles();
#endif
    }

    void set_settings(const geoqik_settings_t& settings,
                      const geoqik_window_settings_t& windowSettings)
    {
        customSettings_ = settings;
        customWindowSettings_ = windowSettings;
        customWindowTitle_ = windowSettings.title ? windowSettings.title : "GeoQik";
        customWindowSettings_.title = customWindowTitle_.c_str();
        hasCustomSettings_ = true;
    }

    void start() {
        if (started_ && is_child_running()) {
            return;
        }
        reset_child_state();
        pipeName_.clear();

        const std::string exePath = resolve_server_executable();
        if (exePath.empty()) {
            throw ServerExecutableNotFoundError(
                "geoqik executable not found. Set GEOQIK_EXE_PATH or add geoqik_server/geoqik to PATH.");
        }

#ifdef _WIN32
        const auto selfPid = static_cast<std::uint64_t>(::GetCurrentProcessId());
#else
        const auto selfPid = static_cast<std::uint64_t>(::getpid());
#endif
        pipeName_ = geoqik::protocol::make_pipe_name(selfPid);
        const auto pipeNameArgument = geoqik::protocol::make_pipe_name_argument(selfPid);

        std::string settingsFilePath;
        if (hasCustomSettings_) {
            settingsFilePath = write_settings_file(customSettings_, customWindowSettings_);
        }

        std::vector<std::string> args{
            exePath,
            "--pipe-name",
            pipeNameArgument,
        };
        if (!settingsFilePath.empty()) {
            args.emplace_back("--settings-file");
            args.emplace_back(settingsFilePath);
        }

        launch_process(exePath, args);

        if (!is_child_running()) {
            reset_child_state();
            throw ServerStartError("geoqik process failed to start.");
        }

        started_ = true;
        hasCustomSettings_ = false;
    }

    [[nodiscard]] const std::string& pipe_name() const noexcept { return pipeName_; }

    [[nodiscard]] bool is_running() { return started_ && is_child_running(); }

    // Blocks until the server process has fully exited, then resets state so the
    // next start() reliably spawns a fresh server after Cleanup / WaitForExit.
    void wait_for_exit() {
        if (started_) {
#ifdef _WIN32
            if (processInfo_.hProcess != nullptr) {
                ::WaitForSingleObject(processInfo_.hProcess, INFINITE);
            }
#else
            if (childPid_ > 0) {
                int status = 0;
                while (::waitpid(childPid_, &status, 0) < 0 && errno == EINTR) {
                }
            }
#endif
        }
        reset_child_state();
        pipeName_.clear();
    }

private:
    ProcessManager() = default;

    [[nodiscard]]
    static std::string write_settings_file(
        const geoqik_settings_t& settings,
        const geoqik_window_settings_t& ws)
    {
#ifdef _WIN32
        char tmpPath[MAX_PATH];
        if (::GetTempPathA(MAX_PATH, tmpPath) == 0) {
            throw ServerStartError("geoqik: GetTempPath failed");
        }
        char tmpFile[MAX_PATH];
        if (::GetTempFileNameA(tmpPath, "gqs", 0, tmpFile) == 0) {
            throw ServerStartError("geoqik: GetTempFileName failed");
        }
        const std::string path(tmpFile);
#else
        char tmp[] = "/tmp/geoqik-settings-XXXXXX";
        const int fd = ::mkstemp(tmp);
        if (fd < 0) {
            throw ServerStartError("geoqik: mkstemp failed");
        }
        ::close(fd);
        const std::string path(tmp);
#endif

        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file) {
            throw ServerStartError("geoqik: cannot write settings file: " + path);
        }

        file.write("GQST", 4);
        file.write(reinterpret_cast<const char*>(&settings), sizeof(settings));

        const std::string title = ws.title ? ws.title : "GeoQik";
        const auto titleLen = static_cast<std::uint32_t>(title.size());
        file.write(reinterpret_cast<const char*>(&titleLen), 4);
        file.write(title.data(), static_cast<std::streamsize>(titleLen));

        auto write_u32 = [&](std::uint32_t v) {
            file.write(reinterpret_cast<const char*>(&v), 4); };
        auto write_i32 = [&](int v) {
            file.write(reinterpret_cast<const char*>(&v), 4); };

        write_u32(ws.width);
        write_u32(ws.height);
        write_i32(ws.red_bits);
        write_i32(ws.green_bits);
        write_i32(ws.blue_bits);
        write_i32(ws.alpha_bits);
        write_i32(ws.depth_bits);
        write_i32(ws.stencil_bits);
        write_i32(ws.accum_red_bits);
        write_i32(ws.accum_green_bits);
        write_i32(ws.accum_blue_bits);
        write_i32(ws.accum_alpha_bits);
        write_i32(ws.aux_buffers);
        write_i32(ws.samples);
        write_i32(ws.refresh_rate);
        write_i32(ws.stereo);
        write_i32(ws.srgb_capable);
        write_i32(ws.double_buffer);
        write_i32(ws.resizable);
        write_i32(ws.visible);
        write_i32(ws.decorated);
        write_i32(ws.focused);
        write_i32(ws.auto_iconify);
        write_i32(ws.floating);
        write_i32(ws.maximized);
        write_i32(ws.center_cursor);
        write_i32(ws.transparent_framebuffer);
        write_i32(ws.focus_on_show);
        write_i32(ws.scale_to_monitor);

        file.close();
        return path;
    }

    [[nodiscard]]
    static std::string get_env(const char* name) {
#ifdef _WIN32
        const DWORD requiredSize = ::GetEnvironmentVariableA(name, nullptr, 0);
        if (requiredSize == 0) {
            return {};
        }

        std::string result(requiredSize, '\0');
        const DWORD copiedSize = ::GetEnvironmentVariableA(
            name, result.data(), requiredSize);
        if (copiedSize == 0) {
            return {};
        }

        result.resize(copiedSize);
        return result;
#else
        const char* value = std::getenv(name);
        return value == nullptr ? std::string{} : std::string(value);
#endif
    }

    [[nodiscard]]
    static bool is_executable_file(const std::string& path) {
        if (path.empty()) {
            return false;
        }
#ifdef _WIN32
        const DWORD attrs = ::GetFileAttributesA(path.c_str());
        return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
#else
        return ::access(path.c_str(), X_OK) == 0;
#endif
    }

    [[nodiscard]]
    static bool has_path_separator(std::string_view value) {
        return value.find('/') != std::string_view::npos
            || value.find('\\') != std::string_view::npos;
    }

    [[nodiscard]]
    static std::string join_path(std::string_view directory, std::string_view file) {
        if (directory.empty()) {
            return std::string(file);
        }
#ifdef _WIN32
        constexpr char separator = '\\';
#else
        constexpr char separator = '/';
#endif
        std::string result(directory);
        if (result.back() != '/' && result.back() != '\\') {
            result.push_back(separator);
        }
        result.append(file);
        return result;
    }

    [[nodiscard]]
    static std::vector<std::string_view> split_path_list(const std::string& paths) {
#ifdef _WIN32
        constexpr char separator = ';';
#else
        constexpr char separator = ':';
#endif
        std::vector<std::string_view> result;
        std::string_view view(paths);
        while (!view.empty()) {
            const std::size_t pos = view.find(separator);
            result.push_back(view.substr(0, pos));
            if (pos == std::string_view::npos) {
                break;
            }
            view.remove_prefix(pos + 1);
        }
        return result;
    }

    [[nodiscard]]
    static std::vector<std::string> executable_names(std::string_view baseName) {
#ifdef _WIN32
        const std::string name(baseName);
        if (name.find('.') != std::string::npos) {
            return {name};
        }
        return {name + ".exe", name};
#else
        return {std::string(baseName)};
#endif
    }

    [[nodiscard]]
    static std::string search_path_for(std::string_view baseName) {
        if (has_path_separator(baseName)) {
            const std::string path(baseName);
            return is_executable_file(path) ? path : std::string{};
        }

        const auto pathEnv = get_env("PATH");
        for (const auto directory : split_path_list(pathEnv)) {
            for (const auto& name : executable_names(baseName)) {
                const std::string candidate = join_path(directory, name);
                if (is_executable_file(candidate)) {
                    return candidate;
                }
            }
        }
        return {};
    }

    [[nodiscard]]
    static std::string resolve_server_executable() {
        if (const auto env = get_env("GEOQIK_EXE_PATH"); !env.empty()) {
            return is_executable_file(env) ? env : std::string{};
        }
        if (auto path = search_path_for("geoqik_server"); !path.empty()) {
            return path;
        }
        return search_path_for("geoqik");
    }

#ifdef _WIN32
    [[nodiscard]]
    static std::string quote_windows_arg(const std::string& arg) {
        if (arg.empty()) {
            return "\"\"";
        }

        bool needsQuotes = false;
        for (const char ch : arg) {
            if (ch == ' ' || ch == '\t' || ch == '"') {
                needsQuotes = true;
                break;
            }
        }
        if (!needsQuotes) {
            return arg;
        }

        std::string quoted;
        quoted.push_back('"');
        std::size_t backslashes = 0;
        for (const char ch : arg) {
            if (ch == '\\') {
                ++backslashes;
            } else if (ch == '"') {
                quoted.append(backslashes * 2 + 1, '\\');
                quoted.push_back(ch);
                backslashes = 0;
            } else {
                quoted.append(backslashes, '\\');
                backslashes = 0;
                quoted.push_back(ch);
            }
        }
        quoted.append(backslashes * 2, '\\');
        quoted.push_back('"');
        return quoted;
    }

    [[nodiscard]]
    static std::string build_command_line(const std::vector<std::string>& args) {
        std::string commandLine;
        for (const auto& arg : args) {
            if (!commandLine.empty()) {
                commandLine.push_back(' ');
            }
            commandLine += quote_windows_arg(arg);
        }
        return commandLine;
    }

    void launch_process(const std::string& exePath, const std::vector<std::string>& args) {
        STARTUPINFOA startupInfo{};
        startupInfo.cb = sizeof(startupInfo);
        PROCESS_INFORMATION processInfo{};
        std::string commandLine = build_command_line(args);

        const BOOL created = ::CreateProcessA(
            exePath.c_str(),
            commandLine.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo);

        if (created == FALSE) {
            throw ServerStartError("geoqik: CreateProcess failed with error " + std::to_string(::GetLastError()));
        }

        processInfo_ = processInfo;
    }

    [[nodiscard]]
    bool is_child_running() {
        if (processInfo_.hProcess == nullptr) {
            return false;
        }
        const DWORD waitResult = ::WaitForSingleObject(processInfo_.hProcess, 0);
        if (waitResult == WAIT_TIMEOUT) {
            return true;
        }
        close_process_handles();
        return false;
    }

    void close_process_handles() noexcept {
        if (processInfo_.hThread != nullptr) {
            ::CloseHandle(processInfo_.hThread);
        }
        if (processInfo_.hProcess != nullptr) {
            ::CloseHandle(processInfo_.hProcess);
        }
        processInfo_ = PROCESS_INFORMATION{};
    }
#else
    [[noreturn]]
    static void exec_child(const std::string& exePath, const std::vector<std::string>& args) {
        std::vector<char*> argv;
        argv.reserve(args.size() + 1);
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        ::execv(exePath.c_str(), argv.data());
        ::_exit(127);
    }

    void launch_process(const std::string& exePath, const std::vector<std::string>& args) {
        const pid_t pid = ::fork();
        if (pid < 0) {
            throw ServerStartError("geoqik: fork failed: " + std::string(std::strerror(errno)));
        }
        if (pid == 0) {
            exec_child(exePath, args);
        }
        childPid_ = pid;
    }

    [[nodiscard]]
    bool is_child_running() {
        if (childPid_ <= 0) {
            return false;
        }

        int status = 0;
        const pid_t result = ::waitpid(childPid_, &status, WNOHANG);
        if (result == 0) {
            return true;
        }
        if (result == childPid_) {
            childPid_ = -1;
            return false;
        }
        if (result < 0 && errno == EINTR) {
            return true;
        }
        childPid_ = -1;
        return false;
    }
#endif

    void reset_child_state() noexcept {
        started_ = false;
#ifdef _WIN32
        close_process_handles();
#else
        childPid_ = -1;
#endif
    }

#ifdef _WIN32
    PROCESS_INFORMATION processInfo_{};
#else
    pid_t childPid_ = -1;
#endif
    std::string pipeName_;
    bool started_ = false;
    bool hasCustomSettings_ = false;
    geoqik_settings_t customSettings_{};
    geoqik_window_settings_t customWindowSettings_{};
    std::string customWindowTitle_;
};

} // namespace geoqik::client::detail

#endif // GEOQIK_CLIENT_DETAIL_PROCESS_MANAGER_HPP
