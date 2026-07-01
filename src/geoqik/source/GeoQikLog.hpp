#ifndef GEOQIKLOG_HPP
#define GEOQIKLOG_HPP

#include "GeoQikMessages.hpp"
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

namespace geoqik
{

[[nodiscard]] std::optional<GeoQikLogEntry> create_log_entry(const GeoQikMessage& message);

void save_log_binary(const std::filesystem::path& path, std::span<const GeoQikLogEntry> entries);
[[nodiscard]] std::vector<GeoQikLogEntry> load_log_binary(const std::filesystem::path& path);

void save_log_json(const std::filesystem::path& path, std::span<const GeoQikLogEntry> entries);
[[nodiscard]] std::vector<GeoQikLogEntry> load_log_json(const std::filesystem::path& path);

} // namespace geoqik

#endif // GEOQIKLOG_HPP
