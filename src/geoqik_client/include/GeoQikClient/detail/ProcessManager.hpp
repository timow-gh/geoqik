#ifndef GEOQIK_CLIENT_DETAIL_PROCESS_MANAGER_HPP
#define GEOQIK_CLIENT_DETAIL_PROCESS_MANAGER_HPP

#include <GeoQikProtocol/Protocol.hpp>

#include <boost/process/v1/child.hpp>
#include <boost/process/v1/search_path.hpp>
#include <cstdint>
#include <cstdlib>
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

        child_ = bp::child(exePath, "--pipe-name", pipeNameArgument);

        if (!child_.running()) {
            throw ServerStartError("geoqik process failed to start.");
        }

        started_ = true;
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
};

} // namespace geoqik::client::detail

#endif // GEOQIK_CLIENT_DETAIL_PROCESS_MANAGER_HPP
