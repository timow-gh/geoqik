#include "GeoQikApiTestBase.hpp"
#include <cmath>
#include <cstring>
#include <tuple>

class TestGeoQikApi_Points : public GeoQikApiTestBase {};

struct RgbaParams {
    float r, g, b, a;
};

class PointColorTest
    : public GeoQikApiTestBase
    , public ::testing::WithParamInterface<RgbaParams> {};

TEST_P(PointColorTest, ColorRoundTrip) {
    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());
    const RgbaParams p = GetParam();
    geoqik_set_point_color(p.r, p.g, p.b, p.a);

    float rr, rg, rb, ra;
    geoqik_get_point_color(&rr, &rg, &rb, &ra);

    EXPECT_FLOAT_EQ(p.r, rr);
    EXPECT_FLOAT_EQ(p.g, rg);
    EXPECT_FLOAT_EQ(p.b, rb);
    EXPECT_FLOAT_EQ(p.a, ra);
    geoqik_cleanup();
}

INSTANTIATE_TEST_SUITE_P(PointColors,
                         PointColorTest,
                         ::testing::Values(RgbaParams{1.0f, 0.0f, 0.0f, 1.0f}, // pure red
                                           RgbaParams{0.0f, 1.0f, 0.0f, 1.0f}, // pure green
                                           RgbaParams{0.0f, 0.0f, 1.0f, 1.0f}, // pure blue
                                           RgbaParams{0.0f, 0.0f, 0.0f, 0.0f}, // fully transparent black
                                           RgbaParams{1.0f, 1.0f, 1.0f, 1.0f}, // opaque white
                                           RgbaParams{0.7f, 0.8f, 0.9f, 0.6f}  // original hardcoded values
                                           ));

TEST_F(TestGeoQikApi_Points, AddPoint) {
    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

    geoqik_result_t result1 = geoqik_add_point(0.0, 0.0, 0.0);
    geoqik_result_t result2 = geoqik_add_point(1.0, 0.0, 0.0);
    geoqik_result_t result3 = geoqik_add_point(0.0, 1.0, 0.0);

    EXPECT_EQ(result1.err, GEOQIK_SUCCESS);
    EXPECT_EQ(result2.err, GEOQIK_SUCCESS);
    EXPECT_EQ(result3.err, GEOQIK_SUCCESS);

    const geoqik_uuid_t nilUuid{};
    EXPECT_NE(0, memcmp(result1.geometryId.value, nilUuid.value, sizeof(nilUuid.value)));
    EXPECT_NE(0, memcmp(result2.geometryId.value, nilUuid.value, sizeof(nilUuid.value)));
    EXPECT_NE(0, memcmp(result3.geometryId.value, nilUuid.value, sizeof(nilUuid.value)));

    geoqik_draw();
    geoqik_cleanup();
}

TEST_F(TestGeoQikApi_Points, AddPointWithHandle) {
    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

    geoqik_draw();

    geoqik_uuid_t pointId1 = geoqik_add_point(0.0, 0.0, 0.0).geometryId;
    geoqik_uuid_t pointId2 = geoqik_add_point(1.0, 0.0, 0.0).geometryId;
    geoqik_uuid_t pointId3 = geoqik_add_point(0.0, 1.0, 0.0).geometryId;

    // Now remove the second point
    geoqik_remove_point(&pointId2);

    geoqik_draw();
    geoqik_cleanup();
}

TEST_F(TestGeoQikApi_Points, CheckMaximumPointsCapacity) {
    geoqik_settings_t settings;
    geoqik_create_default_settings(&settings);
    settings.initialPointCapacity = 3;

    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik(&settings));

    for (int i = 0; i < settings.initialPointCapacity; ++i) {
        geoqik_add_point(static_cast<double>(i) / 10.0, static_cast<double>(i) / 20.0, static_cast<double>(i) / 30.0);
    }
    geoqik_add_point(1.0, 1.0, 1.0); // Adding one more point exceeds the intial capacity
    geoqik_draw();
    geoqik_cleanup();
}

TEST_F(TestGeoQikApi_Points, PointSizeGetSet) {
    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

    float initialSize;
    geoqik_get_point_size(&initialSize);

    const float newSize = 5.0f;
    geoqik_set_point_size(newSize);

    float retrievedSize;
    geoqik_get_point_size(&retrievedSize);
    EXPECT_EQ(newSize, retrievedSize);

    geoqik_cleanup();
}

TEST_F(TestGeoQikApi_Points, AddPointWithColor) {
    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

    const float r = 0.5f, g = 0.6f, b = 0.7f, a = 0.8f;

    geoqik_result_t result1 = geoqik_add_point_with_color(0.0, 0.0, 0.0, r, g, b, a);
    geoqik_result_t result2 = geoqik_add_point_with_color(1.0, 1.0, 1.0, r, g, b, a);

    EXPECT_EQ(result1.err, GEOQIK_SUCCESS);
    EXPECT_EQ(result2.err, GEOQIK_SUCCESS);

    const geoqik_uuid_t nilUuid{};
    EXPECT_NE(0, memcmp(result1.geometryId.value, nilUuid.value, sizeof(nilUuid.value)));
    EXPECT_NE(0, memcmp(result2.geometryId.value, nilUuid.value, sizeof(nilUuid.value)));

    geoqik_draw();
    geoqik_cleanup();
}

TEST_F(TestGeoQikApi_Points, UpdatePointValidation) {
    geoqik_uuid_t id{};

    EXPECT_EQ(geoqik_update_point(nullptr, 1.0, 2.0, 3.0), GEOQIK_ERROR_INVALID_PARAMETER);
    EXPECT_EQ(geoqik_update_point(&id, NAN, 2.0, 3.0), GEOQIK_ERROR_INVALID_PARAMETER);

    const double points[] = {1.0, 2.0};
    EXPECT_EQ(geoqik_update_points_opts(&id, points, 2, nullptr), GEOQIK_ERROR_INVALID_PARAMETER);

    const double validPoints[] = {1.0, 2.0, 3.0};
    const float invalidColors[] = {0.0f, 1.0f, 0.0f};
    geoqik_update_points_options_t opts{};
    opts.color = invalidColors;
    opts.colorCount = 3;
    EXPECT_EQ(geoqik_update_points_opts(&id, validPoints, 3, &opts), GEOQIK_ERROR_WRONG_COLOR_SIZE);

    const float outOfRangeColor[] = {0.0f, 1.1f, 0.0f, 1.0f};
    opts.color = outOfRangeColor;
    opts.colorCount = 4;
    EXPECT_EQ(geoqik_update_points_opts(&id, validPoints, 3, &opts), GEOQIK_ERROR_INVALID_PARAMETER);

    opts.color = nullptr;
    opts.colorCount = 4;
    EXPECT_EQ(geoqik_update_point_opts(&id, 1.0, 2.0, 3.0, &opts), GEOQIK_ERROR_INVALID_PARAMETER);
}

TEST_F(TestGeoQikApi_Points, UpdatePointEnqueues) {
    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

    geoqik_result_t result = geoqik_add_point(0.0, 0.0, 0.0);
    ASSERT_EQ(result.err, GEOQIK_SUCCESS);

    EXPECT_EQ(geoqik_update_point(&result.geometryId, 1.0, 2.0, 3.0), GEOQIK_SUCCESS);
    EXPECT_EQ(geoqik_update_point_with_color(&result.geometryId, 4.0, 5.0, 6.0, 0.1f, 0.2f, 0.3f, 1.0f),
              GEOQIK_SUCCESS);

    geoqik_cleanup();
}

TEST_F(TestGeoQikApi_Points, UpdatePointsEnqueues) {
    ASSERT_EQ(GEOQIK_SUCCESS, init_hidden_geoqik());

    const double initialPoints[] = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
    geoqik_result_t result =
        geoqik_add_points_opts(initialPoints, sizeof(initialPoints) / sizeof(initialPoints[0]), nullptr);
    ASSERT_EQ(result.err, GEOQIK_SUCCESS);

    const double updatedPoints[] = {2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
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
    geoqik_update_points_options_t opts{};
    opts.color = updatedColors;
    opts.colorCount = sizeof(updatedColors) / sizeof(updatedColors[0]);

    EXPECT_EQ(geoqik_update_points_opts(&result.geometryId,
                                        updatedPoints,
                                        sizeof(updatedPoints) / sizeof(updatedPoints[0]),
                                        &opts),
              GEOQIK_SUCCESS);

    geoqik_cleanup();
}
