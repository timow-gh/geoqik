#include "GeoQikApiTestBase.hpp"

#include <cmath>
#include <tuple>

class GeoQikTest_Lines : public GeoQikApiTestBase {};

struct LineRgbaParams {
    float r, g, b, a;
};

class LineColorTest
    : public GeoQikApiTestBase
    , public ::testing::WithParamInterface<LineRgbaParams> {};

TEST_P(LineColorTest, ColorRoundTrip) {
    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());
    const LineRgbaParams p = GetParam();
    geoqik_set_line_color(p.r, p.g, p.b, p.a);

    float rr, rg, rb, ra;
    geoqik_get_line_color(&rr, &rg, &rb, &ra);

    EXPECT_FLOAT_EQ(p.r, rr);
    EXPECT_FLOAT_EQ(p.g, rg);
    EXPECT_FLOAT_EQ(p.b, rb);
    EXPECT_FLOAT_EQ(p.a, ra);
    geoqik_cleanup();
}

INSTANTIATE_TEST_SUITE_P(LineColors,
                         LineColorTest,
                         ::testing::Values(LineRgbaParams{1.0f, 0.0f, 0.0f, 1.0f}, // pure red
                                           LineRgbaParams{0.0f, 1.0f, 0.0f, 1.0f}, // pure green
                                           LineRgbaParams{0.0f, 0.0f, 1.0f, 1.0f}, // pure blue
                                           LineRgbaParams{0.0f, 0.0f, 0.0f, 0.0f}, // fully transparent black
                                           LineRgbaParams{1.0f, 1.0f, 1.0f, 1.0f}, // opaque white
                                           LineRgbaParams{0.2f, 0.3f, 0.4f, 0.5f}  // original hardcoded values
                                           ));

TEST_F(GeoQikTest_Lines, AddLine) {
    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

    geoqik_add_line(0.0, 0.0, 0.0, 1.0, 0.0, 0.0);
    geoqik_add_line(0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    geoqik_add_line(0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

    geoqik_draw();
    geoqik_cleanup();
}

TEST_F(GeoQikTest_Lines, CheckMaximumLinesCapacity) {
    geoqik_settings_t settings;
    geoqik_create_default_settings(&settings);
    settings.initialLineCapacity = 3;

    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik(&settings));

    for (int i = 0; i < settings.initialLineCapacity; ++i) {
        geoqik_add_line(static_cast<double>(i) / 10.0,
                        static_cast<double>(i) / 20.0,
                        static_cast<double>(i) / 30.0,
                        static_cast<double>(i + 1) / 10.0,
                        static_cast<double>(i + 1) / 20.0,
                        static_cast<double>(i + 1) / 30.0);
    }
    geoqik_add_line(1.0, 1.0, 1.0, 2.0, 2.0, 2.0);
    geoqik_draw();
    geoqik_cleanup();
}

TEST_F(GeoQikTest_Lines, LineWidthGetSet) {
    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

    float initialWidth;
    geoqik_get_line_width(&initialWidth);

    const float newWidth = 3.0f;
    geoqik_set_line_width(newWidth);

    float retrievedWidth;
    geoqik_get_line_width(&retrievedWidth);
    EXPECT_FLOAT_EQ(newWidth, retrievedWidth);
    geoqik_cleanup();
}

TEST_F(GeoQikTest_Lines, AddLineWithColor) {
    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

    geoqik_add_line_with_color(0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0f, 0.0f, 0.0f, 1.0f); // Red line
    geoqik_add_line_with_color(1.0, 1.0, 1.0, 2.0, 1.0, 1.0, 0.0f, 1.0f, 0.0f, 1.0f); // Green line

    geoqik_draw();
    geoqik_cleanup();
}

TEST_F(GeoQikTest_Lines, AddLineWithIdempotency) {
    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

    geoqik_uuid_t key;
    geoqik_generate_uuid(&key);

    geoqik_add_line_opts_t opts{};
    opts.idempotencyKey = key;

    geoqik_result_t first = geoqik_add_line_opts(0.0, 0.0, 0.0, 1.0, 0.0, 0.0, &opts);
    EXPECT_EQ(first.err, GEOQIK_SUCCESS);

    geoqik_result_t second = geoqik_add_line_opts(0.0, 0.0, 0.0, 1.0, 0.0, 0.0, &opts);
    EXPECT_EQ(second.err, GEOQIK_SUCCESS);

    geoqik_draw();
    geoqik_cleanup();
}

TEST_F(GeoQikTest_Lines, AddLineWithColorOpts) {
    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

    const float color[4] = {0.0f, 0.0f, 1.0f, 1.0f}; // Blue
    geoqik_add_line_opts_t opts{};
    opts.color = color;
    opts.colorCount = 4;

    geoqik_result_t result = geoqik_add_line_opts(0.0, 0.0, 0.0, 1.0, 1.0, 1.0, &opts);
    EXPECT_EQ(result.err, GEOQIK_SUCCESS);

    geoqik_draw();
    geoqik_cleanup();
}

TEST_F(GeoQikTest_Lines, AddLinesWithOpts) {
    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

    // Three lines, each with 6 doubles (x1,y1,z1, x2,y2,z2)
    const double lines[] = {
        0.0,
        0.0,
        0.0,
        1.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        1.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        1.0,
    };
    // One color per line (RGBA * 3 lines = 12 floats)
    const float colors[] = {
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        1.0f,
    };

    geoqik_add_line_opts_t opts{};
    opts.color = colors;
    opts.colorCount = 12;

    geoqik_result_t result = geoqik_add_lines_opts(lines, sizeof(lines) / sizeof(lines[0]), &opts);
    EXPECT_EQ(result.err, GEOQIK_SUCCESS);

    geoqik_draw();
    geoqik_cleanup();
}

TEST_F(GeoQikTest_Lines, UpdateLineValidation) {
    geoqik_uuid_t id{};

    EXPECT_EQ(geoqik_update_line(nullptr, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0), GEOQIK_ERROR_INVALID_PARAMETER);
    EXPECT_EQ(geoqik_update_line(&id, 0.0, 0.0, 0.0, NAN, 1.0, 1.0), GEOQIK_ERROR_INVALID_PARAMETER);

    const double lines[] = {0.0, 0.0, 0.0, 1.0, 1.0};
    EXPECT_EQ(geoqik_update_lines_opts(&id, lines, 5, nullptr), GEOQIK_ERROR_INVALID_PARAMETER);

    const double validLines[] = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
    const float invalidColors[] = {0.0f, 1.0f, 0.0f};
    geoqik_update_line_opts_t opts{};
    opts.color = invalidColors;
    opts.colorCount = 3;
    EXPECT_EQ(geoqik_update_lines_opts(&id, validLines, 6, &opts), GEOQIK_ERROR_WRONG_COLOR_SIZE);

    const float outOfRangeColor[] = {0.0f, -0.1f, 0.0f, 1.0f};
    opts.color = outOfRangeColor;
    opts.colorCount = 4;
    EXPECT_EQ(geoqik_update_lines_opts(&id, validLines, 6, &opts), GEOQIK_ERROR_INVALID_PARAMETER);

    opts.color = nullptr;
    opts.colorCount = 4;
    EXPECT_EQ(geoqik_update_line_opts(&id, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, &opts), GEOQIK_ERROR_INVALID_PARAMETER);

    EXPECT_EQ(geoqik_remove_line(nullptr), GEOQIK_ERROR_INVALID_PARAMETER);
}

TEST_F(GeoQikTest_Lines, UpdateLineEnqueues) {
    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

    geoqik_result_t result = geoqik_add_line_opts(0.0, 0.0, 0.0, 1.0, 1.0, 1.0, nullptr);
    ASSERT_EQ(result.err, GEOQIK_SUCCESS);

    EXPECT_EQ(geoqik_update_line(&result.geometryId, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0), GEOQIK_SUCCESS);
    EXPECT_EQ(
        geoqik_update_line_with_color(&result.geometryId, 8.0, 9.0, 10.0, 11.0, 12.0, 13.0, 0.1f, 0.2f, 0.3f, 1.0f),
        GEOQIK_SUCCESS);

    geoqik_cleanup();
}

TEST_F(GeoQikTest_Lines, UpdateLinesAndRemoveLineEnqueue) {
    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

    const double initialLines[] = {
        0.0,
        0.0,
        0.0,
        1.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        1.0,
        0.0,
    };
    geoqik_result_t result =
        geoqik_add_lines_opts(initialLines, sizeof(initialLines) / sizeof(initialLines[0]), nullptr);
    ASSERT_EQ(result.err, GEOQIK_SUCCESS);

    const double updatedLines[] = {
        1.0,
        1.0,
        1.0,
        2.0,
        1.0,
        1.0,
        1.0,
        1.0,
        1.0,
        1.0,
        2.0,
        1.0,
    };
    const float updatedColors[] = {
        1.0f,
        0.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
        0.0f,
        1.0f,
    };
    geoqik_update_line_opts_t opts{};
    opts.color = updatedColors;
    opts.colorCount = sizeof(updatedColors) / sizeof(updatedColors[0]);

    EXPECT_EQ(geoqik_update_lines_opts(&result.geometryId,
                                       updatedLines,
                                       sizeof(updatedLines) / sizeof(updatedLines[0]),
                                       &opts),
              GEOQIK_SUCCESS);
    EXPECT_EQ(geoqik_remove_line(&result.geometryId), GEOQIK_SUCCESS);

    geoqik_cleanup();
}

TEST_F(GeoQikTest_Lines, StyledStrokeEnqueuesUpdateAndRemove) {
    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());
    const double lines[] = {0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 2.0, 1.0, 0.0};
    const float dash[] = {4.0f, 2.0f};
    const std::uint8_t flags[] = {1, 1, 0, 1};
    geoqik_add_line_opts_t opts{};
    opts.styleSet = 1;
    opts.style.lineWidth = 5.0f;
    opts.style.cap = GEOQIK_LINE_CAP_ROUND;
    opts.style.join = GEOQIK_LINE_JOIN_BEVEL;
    opts.style.miterLimit = 6.0f;
    opts.style.dashPattern = dash;
    opts.style.dashPatternCount = std::size(dash);
    opts.style.dashPhase = 0.25f;
    opts.style.dashSpace = GEOQIK_DASH_SPACE_SCREEN;
    opts.lineType = GEOQIK_LINE_TYPE_LINE_STRIP;
    opts.perVertexDashFlags = flags;
    opts.perVertexDashFlagCount = std::size(flags);

    const auto result = geoqik_add_lines_opts(lines, std::size(lines), &opts);
    ASSERT_EQ(GEOQIK_SUCCESS, result.err);
    geoqik_update_line_opts_t updateOpts{};
    updateOpts.styleSet = 1;
    updateOpts.style = opts.style;
    updateOpts.style.dashSpace = GEOQIK_DASH_SPACE_WORLD;
    updateOpts.lineType = GEOQIK_LINE_TYPE_LINE_LOOP;
    updateOpts.perVertexDashFlags = flags;
    updateOpts.perVertexDashFlagCount = std::size(flags);
    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_update_lines_opts(&result.geometryId, lines, std::size(lines), &updateOpts));
    EXPECT_EQ(GEOQIK_SUCCESS, geoqik_remove_line(&result.geometryId));
    geoqik_cleanup();
}

TEST_F(GeoQikTest_Lines, StyledStrokeRejectsInvalidFlagsAndEnums) {
    const double line[] = {0.0, 0.0, 0.0, 1.0, 0.0, 0.0};
    const std::uint8_t flag = 1;
    geoqik_add_line_opts_t opts{};
    opts.styleSet = 1;
    opts.perVertexDashFlags = &flag;
    opts.perVertexDashFlagCount = 1;
    EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_add_lines_opts(line, std::size(line), &opts).err);

    opts.perVertexDashFlagCount = 0;
    opts.perVertexDashFlags = nullptr;
    opts.style.cap = static_cast<geoqik_line_cap_t>(99);
    EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_add_lines_opts(line, std::size(line), &opts).err);

    const std::uint8_t invalidFlags[] = {1, 2};
    opts.style.cap = GEOQIK_LINE_CAP_BUTT;
    opts.perVertexDashFlags = invalidFlags;
    opts.perVertexDashFlagCount = std::size(invalidFlags);
    EXPECT_EQ(GEOQIK_ERROR_INVALID_PARAMETER, geoqik_add_lines_opts(line, std::size(line), &opts).err);
}
