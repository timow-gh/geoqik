#ifndef GEOQIKMESSAGEJSONSERIALIZATION_HPP
#define GEOQIKMESSAGEJSONSERIALIZATION_HPP

#include "GeoQikMessages.hpp"
#include <nlohmann/json_fwd.hpp> // forward-declares nlohmann::json; keeps the full header out of this header

namespace geoqik
{

[[nodiscard]] nlohmann::json to_json(const GeoQikLogEntry& message);
[[nodiscard]] GeoQikLogEntry from_json(const nlohmann::json& value);

} // namespace geoqik

#endif // GEOQIKMESSAGEJSONSERIALIZATION_HPP
