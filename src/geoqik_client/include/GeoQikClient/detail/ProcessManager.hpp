#pragma once

#include <GeoQikProtocol/Protocol.hpp>
#include <boost/process.hpp>
#include <stdexcept>
#include <string>
#include <cstdlib>

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
        if (const char* env = std::getenv("GEOQIK_EXE_PATH")) {
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

    bool is_running() const {
        return child_.running();
    }

private:
    ProcessManager() = default;
    boost::process::child child_;
    std::string pipeName_;
};

} // namespace geoqik::client::detail
