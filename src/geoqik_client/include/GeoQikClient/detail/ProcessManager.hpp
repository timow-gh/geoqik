#ifndef GEOQIK_CLIENT_DETAIL_PROCESS_MANAGER_HPP
#define GEOQIK_CLIENT_DETAIL_PROCESS_MANAGER_HPP

#include <GeoQikClient/detail/ClientTypes.hpp>
#include <GeoQikProtocol/Protocol.hpp>

#include <boost/process/v1/child.hpp>
#include <boost/process/v1/search_path.hpp>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
#include <system_error>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#else
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
    ~ProcessManager() = default;

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
        if (started_ && child_.running()) {
            return;
        }
        started_ = false;
        pipeName_.clear();

        namespace bp = boost::process::v1;

        boost::filesystem::path exePath;
        if (const auto env = get_env("GEOQIK_EXE_PATH"); !env.empty()) {
            exePath = env;
        } else {
            exePath = bp::search_path("geoqik");
        }
        if (exePath.empty() || !boost::filesystem::exists(exePath)) {
            throw ServerExecutableNotFoundError(
                "geoqik executable not found. Set GEOQIK_EXE_PATH or add geoqik to PATH.");
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

        if (settingsFilePath.empty()) {
            child_ = bp::child(exePath, "--pipe-name", pipeNameArgument);
        } else {
            child_ = bp::child(exePath, "--pipe-name", pipeNameArgument,
                               "--settings-file", settingsFilePath);
        }

        if (!child_.running()) {
            throw ServerStartError("geoqik process failed to start.");
        }

        started_ = true;
        hasCustomSettings_ = false;
    }

    [[nodiscard]] const std::string& pipe_name() const noexcept { return pipeName_; }

    [[nodiscard]] bool is_running() { return started_ && child_.running(); }

    // Blocks until the server process has fully exited, then resets state so the
    // next start() reliably spawns a fresh server. Must be called after a
    // terminating command (Cleanup / WaitForExit) tells the server to exit:
    // boost::process reports a just-signalled child as still running for a short
    // window, so without an explicit wait the next start() can skip spawning and
    // a later call connects to a leftover pipe instance of the dead server.
    void wait_for_exit() {
        if (started_) {
            std::error_code ec;
            child_.wait(ec);
        }
        started_ = false;
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

    boost::process::v1::child child_;
    std::string pipeName_;
    bool started_ = false;
    bool hasCustomSettings_ = false;
    geoqik_settings_t customSettings_{};
    geoqik_window_settings_t customWindowSettings_{};
    std::string customWindowTitle_;
};

} // namespace geoqik::client::detail

#endif // GEOQIK_CLIENT_DETAIL_PROCESS_MANAGER_HPP
