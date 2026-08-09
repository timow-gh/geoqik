#ifndef GEOQIK_SOURCE_GEOMETRYBUFFERS_GEOMETRYSTYLE_HPP
#define GEOQIK_SOURCE_GEOMETRYBUFFERS_GEOMETRYSTYLE_HPP

#include <cstdint>
#include <vector>

namespace geoqik {

struct StrokeStyleData {
    float lineWidth{0.0F};
    std::uint8_t cap{0};
    std::uint8_t join{0};
    float miterLimit{0.0F};
    std::vector<float> dashPattern;
    float dashPhase{0.0F};
    std::uint8_t dashSpace{0};

    [[nodiscard]] bool operator==(const StrokeStyleData&) const = default;
};

struct StyledLineData {
    std::vector<float> vertices;
    std::vector<float> colors;
    StrokeStyleData style;
    std::uint8_t lineType{0};
    std::vector<std::uint8_t> perVertexDashFlags;

    [[nodiscard]] bool operator==(const StyledLineData&) const = default;
};

struct StyledPointData {
    std::vector<float> points;
    std::vector<float> colors;
    std::vector<float> radii;
    std::uint8_t sizeSpace{0};

    [[nodiscard]] bool operator==(const StyledPointData&) const = default;
};

} // namespace geoqik

#endif // GEOQIK_SOURCE_GEOMETRYBUFFERS_GEOMETRYSTYLE_HPP
