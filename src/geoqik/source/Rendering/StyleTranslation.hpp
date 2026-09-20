#ifndef GEOQIK_SOURCE_RENDERING_STYLETRANSLATION_HPP
#define GEOQIK_SOURCE_RENDERING_STYLETRANSLATION_HPP

#include "GeometryBuffers/GeometryStyle.hpp"

#include <plinth/DashSpace.hpp>
#include <plinth/LineType.hpp>
#include <plinth/SphereSizeSpace.hpp>
#include <plinth/StrokeStyle.hpp>

#include <GeoQik/ApiTypes.h>
#include <cstdint>

namespace geoqik {

[[nodiscard]] inline renderer::LineCap to_plinth_cap(std::uint8_t value) {
    switch (value) {
    case GEOQIK_LINE_CAP_SQUARE: return renderer::LineCap::Square;
    case GEOQIK_LINE_CAP_ROUND:  return renderer::LineCap::Round;
    case GEOQIK_LINE_CAP_BUTT:
    default:                     return renderer::LineCap::Butt;
    }
}

[[nodiscard]] inline renderer::LineJoin to_plinth_join(std::uint8_t value) {
    switch (value) {
    case GEOQIK_LINE_JOIN_BEVEL: return renderer::LineJoin::Bevel;
    case GEOQIK_LINE_JOIN_ROUND: return renderer::LineJoin::Round;
    case GEOQIK_LINE_JOIN_MITER:
    default:                     return renderer::LineJoin::Miter;
    }
}

[[nodiscard]] inline renderer::DashSpace to_plinth_dash_space(std::uint8_t value) {
    switch (value) {
    case GEOQIK_DASH_SPACE_SCREEN: return renderer::DashSpace::Screen;
    case GEOQIK_DASH_SPACE_WORLD:
    default:                       return renderer::DashSpace::World;
    }
}

[[nodiscard]] inline renderer::SphereSizeSpace to_plinth_size_space(std::uint8_t value) {
    switch (value) {
    case GEOQIK_SPHERE_SIZE_SPACE_WORLD:  return renderer::SphereSizeSpace::World;
    case GEOQIK_SPHERE_SIZE_SPACE_SCREEN:
    default:                              return renderer::SphereSizeSpace::Screen;
    }
}

[[nodiscard]] inline renderer::LineType to_plinth_line_type(std::uint8_t value) {
    switch (value) {
    case GEOQIK_LINE_TYPE_LINE_STRIP: return renderer::LineType::line_strip();
    case GEOQIK_LINE_TYPE_LINE_LOOP:  return renderer::LineType::line_loop();
    case GEOQIK_LINE_TYPE_LINES:
    default:                          return renderer::LineType::lines();
    }
}

// Builds a plinth StrokeStyle from GeoQik's internal StrokeStyleData, applying the same
// fallbacks used across the API: a non-positive lineWidth falls back to fallbackWidth, and a
// non-positive miterLimit falls back to plinth's SVG default (4.0). Shared by standalone styled
// lines and mesh segment overlays so both honour cap/join/miter/dash/depthLayer identically.
[[nodiscard]] inline renderer::StrokeStyle build_stroke_style(const StrokeStyleData& data, float fallbackWidth) {
    constexpr float defaultMiterLimit = 4.0F;
    renderer::StrokeStyle style;
    style.lineWidth = data.lineWidth > 0.0F ? data.lineWidth : fallbackWidth;
    style.cap = to_plinth_cap(data.cap);
    style.join = to_plinth_join(data.join);
    style.miterLimit = data.miterLimit > 0.0F ? data.miterLimit : defaultMiterLimit;
    style.dashPattern = data.dashPattern;
    style.dashPhase = data.dashPhase;
    style.dashSpace = to_plinth_dash_space(data.dashSpace);
    style.depthLayer = data.depthLayer;
    return style;
}

} // namespace geoqik

#endif // GEOQIK_SOURCE_RENDERING_STYLETRANSLATION_HPP
