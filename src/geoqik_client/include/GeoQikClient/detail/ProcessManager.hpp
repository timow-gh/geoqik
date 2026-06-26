#pragma once

#include <GeoQikProtocol/Protocol.hpp>

#include <boost/process.hpp>
#include <cstdlib>
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
    static ProcessManager& instance() {
        static ProcessManager pm;
        return pm;
    }

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
        const int selfPid = static_cast<int>(::GetCurrentProcessId());
#else
        const int selfPid = static_cast<int>(::getpid());
#endif
        pipeName_ = geoqik::protocol::make_pipe_name(selfPid);

        child_ = bp::child(exePath, "--pipe-name", pipeName_);

        if (!child_.running()) {
            throw std::runtime_error("geoqik process failed to start.");
        }
    }

    const std::string& pipe_name() const { return pipeName_; }

    bool is_running() { return child_.running(); }

private:
    ProcessManager() = default;

    static std::string get_env(const char* name) {
#ifdef _WIN32
        char* value = nullptr;
        std::size_t size = 0;
        if (_dupenv_s(&value, &size, name) != 0 || value == nullptr) {
            return {};
        }

        std::string result(value);
        std::free(value);
        return result;
#else
        const char* value = std::getenv(name);
        return value == nullptr ? std::string{} : std::string(value);
#endif
    }

    boost::process::child child_;
    std::string pipeName_;
};

} // namespace geoqik::client::detail
