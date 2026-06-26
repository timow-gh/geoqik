#pragma once

#include <GeoQikProtocol/Protocol.hpp>

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
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
#  include <csignal>
#  include <sys/types.h>
#  include <sys/wait.h>
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
        std::filesystem::path exePath;
        if (const auto env = get_env("GEOQIK_EXE_PATH"); !env.empty()) {
            exePath = env;
        } else {
            exePath = find_executable("geoqik");
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

        child_ = ProcessHandle::start(exePath, pipeName_);

        if (!child_.running()) {
            throw std::runtime_error("geoqik process failed to start.");
        }
    }

    const std::string& pipe_name() const { return pipeName_; }

    bool is_running() const { return child_.running(); }

private:
    class ProcessHandle {
    public:
        ProcessHandle() = default;

        ProcessHandle(const ProcessHandle&) = delete;
        ProcessHandle& operator=(const ProcessHandle&) = delete;

        ProcessHandle(ProcessHandle&& other) noexcept { *this = std::move(other); }

        ProcessHandle& operator=(ProcessHandle&& other) noexcept {
            if (this != &other) {
                close();
#ifdef _WIN32
                process_ = other.process_;
                other.process_ = nullptr;
#else
                pid_ = other.pid_;
                other.pid_ = -1;
#endif
            }
            return *this;
        }

        ~ProcessHandle() { close(); }

        static ProcessHandle start(const std::filesystem::path& exePath, const std::string& pipeName) {
            ProcessHandle handle;

#ifdef _WIN32
            std::string commandLine = quote_argument(exePath.string()) + " --pipe-name " + quote_argument(pipeName);
            STARTUPINFOA startupInfo{};
            startupInfo.cb = sizeof(startupInfo);
            PROCESS_INFORMATION processInfo{};

            std::vector<char> writableCommandLine(commandLine.begin(), commandLine.end());
            writableCommandLine.push_back('\0');

            if (!::CreateProcessA(
                    nullptr,
                    writableCommandLine.data(),
                    nullptr,
                    nullptr,
                    FALSE,
                    0,
                    nullptr,
                    nullptr,
                    &startupInfo,
                    &processInfo)) {
                throw std::runtime_error("failed to start geoqik process.");
            }

            ::CloseHandle(processInfo.hThread);
            handle.process_ = processInfo.hProcess;
#else
            const pid_t pid = ::fork();
            if (pid < 0) {
                throw std::runtime_error("failed to start geoqik process.");
            }
            if (pid == 0) {
                ::execl(exePath.c_str(), exePath.c_str(), "--pipe-name", pipeName.c_str(), nullptr);
                _exit(127);
            }
            handle.pid_ = pid;
#endif

            return handle;
        }

        bool running() const {
#ifdef _WIN32
            return process_ != nullptr && ::WaitForSingleObject(process_, 0) == WAIT_TIMEOUT;
#else
            if (pid_ <= 0) {
                return false;
            }

            int status = 0;
            const pid_t result = ::waitpid(pid_, &status, WNOHANG);
            return result == 0;
#endif
        }

    private:
        void close() {
#ifdef _WIN32
            if (process_ != nullptr) {
                ::CloseHandle(process_);
                process_ = nullptr;
            }
#else
            if (pid_ > 0) {
                int status = 0;
                (void)::waitpid(pid_, &status, WNOHANG);
                pid_ = -1;
            }
#endif
        }

#ifdef _WIN32
        HANDLE process_{nullptr};
#else
        pid_t pid_{-1};
#endif
    };

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

    static std::filesystem::path find_executable(std::string_view name) {
#ifdef _WIN32
        for (const std::string candidate : {std::string(name) + ".exe", std::string(name)}) {
            std::vector<char> buffer(MAX_PATH);
            const DWORD length = ::SearchPathA(
                nullptr,
                candidate.c_str(),
                nullptr,
                static_cast<DWORD>(buffer.size()),
                buffer.data(),
                nullptr);
            if (length > 0) {
                if (length >= buffer.size()) {
                    buffer.resize(length + 1);
                    if (::SearchPathA(
                            nullptr,
                            candidate.c_str(),
                            nullptr,
                            static_cast<DWORD>(buffer.size()),
                            buffer.data(),
                            nullptr) == 0) {
                        continue;
                    }
                }
                return buffer.data();
            }
        }
        return {};
#else
        const std::string pathEnv = get_env("PATH");
        if (pathEnv.empty()) {
            return {};
        }

        std::string_view path(pathEnv);
        while (!path.empty()) {
            const std::size_t separator = path.find(':');
            const std::string_view entry = path.substr(0, separator);
            const std::filesystem::path candidate = std::filesystem::path(entry) / name;
            if (std::filesystem::exists(candidate)) {
                return candidate;
            }
            if (separator == std::string_view::npos) {
                break;
            }
            path.remove_prefix(separator + 1);
        }
        return {};
#endif
    }

#ifdef _WIN32
    static std::string quote_argument(const std::string& argument) {
        std::string quoted;
        quoted.reserve(argument.size() + 2);
        quoted.push_back('"');
        for (const char ch : argument) {
            if (ch == '"') {
                quoted.push_back('\\');
            }
            quoted.push_back(ch);
        }
        quoted.push_back('"');
        return quoted;
    }
#endif

    ProcessManager() = default;
    ProcessHandle child_;
    std::string pipeName_;
};

} // namespace geoqik::client::detail
