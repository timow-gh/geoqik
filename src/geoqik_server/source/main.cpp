#include "IpcServer.hpp"
#include <GeoQik/GeoQik.hpp>
#include <GeoQikProtocol/Protocol.hpp>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    std::string pipeNameArgument;
    std::string settingsFilePath;

    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == "--pipe-name") {
            pipeNameArgument = argv[i + 1];
        } else if (std::string(argv[i]) == "--settings-file") {
            settingsFilePath = argv[i + 1];
        }
    }

    if (pipeNameArgument.empty()) {
        std::cerr << "geoqik_server: --pipe-name <name> is required\n";
        return 1;
    }
    const auto pipeName = geoqik::protocol::make_pipe_name_from_argument(pipeNameArgument);

    geoqik_error_code_t err = GEOQIK_SUCCESS;

    if (!settingsFilePath.empty()) {
        std::ifstream file(settingsFilePath, std::ios::binary);
        if (!file) {
            std::cerr << "geoqik_server: cannot open settings file: " << settingsFilePath << '\n';
            return 1;
        }
        file.seekg(0, std::ios::end);
        const std::size_t fileSize = static_cast<std::size_t>(file.tellg());
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(fileSize);
        file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
        file.close();
        std::remove(settingsFilePath.c_str());

        std::size_t offset = 0;
        if (fileSize < 4 || std::string(buffer.data(), 4) != "GQST") {
            std::cerr << "geoqik_server: invalid settings file\n";
            return 1;
        }
        offset += 4;

        geoqik_settings_t settings{};
        geoqik_create_default_settings(&settings);
        if (fileSize >= offset + sizeof(geoqik_settings_t)) {
            std::memcpy(&settings, buffer.data() + offset, sizeof(geoqik_settings_t));
            offset += sizeof(geoqik_settings_t);
        }

        geoqik_window_settings_t windowSettings{};
        geoqik_init_default_window_settings(&windowSettings);
        static std::string titleStorage;
        if (offset + 4 <= fileSize) {
            std::uint32_t titleLen = 0;
            std::memcpy(&titleLen, buffer.data() + offset, 4);
            offset += 4;
            if (offset + titleLen <= fileSize) {
                titleStorage = std::string(buffer.data() + offset, titleLen);
                windowSettings.title = titleStorage.c_str();
                offset += titleLen;
            }
        }
        auto read_u32 = [&](std::uint32_t& dest) {
            if (offset + 4 <= fileSize) {
                std::memcpy(&dest, buffer.data() + offset, 4);
                offset += 4;
            }
        };
        auto read_i32 = [&](int& dest) {
            if (offset + 4 <= fileSize) {
                std::memcpy(&dest, buffer.data() + offset, 4);
                offset += 4;
            }
        };
        read_u32(windowSettings.width);
        read_u32(windowSettings.height);
        read_i32(windowSettings.red_bits);
        read_i32(windowSettings.green_bits);
        read_i32(windowSettings.blue_bits);
        read_i32(windowSettings.alpha_bits);
        read_i32(windowSettings.depth_bits);
        read_i32(windowSettings.stencil_bits);
        read_i32(windowSettings.accum_red_bits);
        read_i32(windowSettings.accum_green_bits);
        read_i32(windowSettings.accum_blue_bits);
        read_i32(windowSettings.accum_alpha_bits);
        read_i32(windowSettings.aux_buffers);
        read_i32(windowSettings.samples);
        read_i32(windowSettings.refresh_rate);
        read_i32(windowSettings.stereo);
        read_i32(windowSettings.srgb_capable);
        read_i32(windowSettings.double_buffer);
        read_i32(windowSettings.resizable);
        read_i32(windowSettings.visible);
        read_i32(windowSettings.decorated);
        read_i32(windowSettings.focused);
        read_i32(windowSettings.auto_iconify);
        read_i32(windowSettings.floating);
        read_i32(windowSettings.maximized);
        read_i32(windowSettings.center_cursor);
        read_i32(windowSettings.transparent_framebuffer);
        read_i32(windowSettings.focus_on_show);
        read_i32(windowSettings.scale_to_monitor);

        err = geoqik_init_with_settings(&settings, &windowSettings);
    } else {
        err = geoqik_init();
    }

    if (err != GEOQIK_SUCCESS) {
        std::cerr << "geoqik_server: init failed: " << geoqik_get_error_string(err) << '\n';
        return 1;
    }

    geoqik::server::run(pipeName);
    return 0;
}
