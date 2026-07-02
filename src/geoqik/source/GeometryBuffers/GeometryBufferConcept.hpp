#ifndef GEOMETRYBUFFERCONCEPT_HPP
#define GEOMETRYBUFFERCONCEPT_HPP

#include "GeoQikSettings.hpp"
#include <Core/UUID.hpp>
#include <concepts>
#include <memory>

namespace geoqik {

template <typename T>
concept GeometryBuffer = requires(T buffer, const GeoQikSettings& settings) {
    { std::declval<T>().create(settings) } -> std::same_as<std::unique_ptr<T>>;
    { std::declval<T>().get_default_color() } -> std::same_as<Color>;
    { std::declval<T>().set_default_color(1.0f, 1.0f, 1.0f, 1.0f) } -> std::same_as<void>;
    { std::declval<T>().translate_geometry(core::UUID{}, 0.0f, 0.0f, 0.0f) } -> std::same_as<void>;
    {
        std::declval<T>().rotate_geometry(core::UUID{}, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 90.0f)
    } -> std::same_as<void>;
};

} // namespace geoqik

#endif // GEOMETRYBUFFERCONCEPT_HPP
