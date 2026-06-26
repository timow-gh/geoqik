#pragma once

#include <GeoQikProtocol/Protocol.hpp>

#include <boost/process.hpp>
#include <cstdlib>
#include <cstdint>
#include <stdexcept>
#include <string>

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
        namespace bp = boost::process;

        boost::filesystem::path exePath;
        if (const auto env = get_env("GEOQIK_EXE_PATH"); !env.empty()) {
            exePath = env;
        } else {
            exePath = bp::search_path("geoqik");
        }
        if (exePath.empty()) {
            throw std::runtime_error(
                "geoqik executable not found. Set GEOQIK_EXE_PATH or add geoqik to PATH.");
        }

#ifdef _WIN32
        const auto selfPid = static_cast<std::uint64_t>(::GetCurrentProcessId());
#else
        const auto selfPid = static_cast<std::uint64_t>(::getpid());
#endif
        pipeName_ = geoqik::protocol::make_pipe_name(selfPid);

        child_ = bp::child(exePath, "--pipe-name", pipeName_);

        if (!child_.running()) {
            throw std::runtime_error("geoqik process failed to start.");
        }
    }

    [[nodiscard]] const std::string& pipe_name() const noexcept { return pipeName_; }

    [[nodiscard]] bool is_running() { return child_.running(); }

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

    boost::process::child child_{};
    std::string pipeName_;
};

} // namespace geoqik::client::detail
