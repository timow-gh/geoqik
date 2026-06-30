#include <GeoQikClient/GeoQikClient.hpp>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

namespace {
constexpr float red[]   = {1.0f, 0.0f, 0.0f, 1.0f};
constexpr float green[] = {0.0f, 1.0f, 0.0f, 1.0f};
constexpr float blue[]  = {0.0f, 0.0f, 1.0f, 1.0f};
} // namespace

int main() {
    [[maybe_unused]] auto err = geoqik_init();
    assert(err == GEOQIK_SUCCESS && "geoqik_init failed; is GEOQIK_EXE_PATH set?");

    err = geoqik_draw();
    assert(err == GEOQIK_SUCCESS);

    auto pr = geoqik_add_point_with_color(0.0, 0.0, 0.0, 1.0f, 0.0f, 0.0f, 1.0f);
    assert(pr.err == GEOQIK_SUCCESS);

    err = geoqik_update_point(&pr.geometryId, 0.1, 0.0, 0.0);
    assert(err == GEOQIK_SUCCESS);

    err = geoqik_update_point_with_color(&pr.geometryId, 0.2, 0.0, 0.0,
        0.0f, 1.0f, 0.0f, 1.0f);
    assert(err == GEOQIK_SUCCESS);

    geoqik_add_points_options_t pOpts{};
    pOpts.color      = red;
    pOpts.colorCount = 4;
    auto pr2 = geoqik_add_point_opts(1.0, 0.0, 0.0, &pOpts);
    assert(pr2.err == GEOQIK_SUCCESS);

    geoqik_update_points_options_t upOpts{};
    upOpts.color      = blue;
    upOpts.colorCount = 4;
    err = geoqik_update_point_opts(&pr2.geometryId, 1.1, 0.0, 0.0, &upOpts);
    assert(err == GEOQIK_SUCCESS);

    constexpr std::size_t batchSize = 5;
    std::array<double, batchSize * 3> batchPts{};
    std::array<float, batchSize * 4>  batchColors{};
    for (std::size_t i = 0; i < batchSize; ++i) {
        batchPts[i * 3]     = static_cast<double>(i) * 0.3;
        batchPts[i * 3 + 1] = 1.0;
        batchPts[i * 3 + 2] = 0.0;
        batchColors[i * 4]     = static_cast<float>(i) / static_cast<float>(batchSize);
        batchColors[i * 4 + 1] = 0.5f;
        batchColors[i * 4 + 2] = 1.0f - static_cast<float>(i) / static_cast<float>(batchSize);
        batchColors[i * 4 + 3] = 1.0f;
    }
    geoqik_add_points_options_t batchOpts{};
    batchOpts.color      = batchColors.data();
    batchOpts.colorCount = batchColors.size();
    auto bpr = geoqik_add_points_opts(batchPts.data(), batchPts.size(), &batchOpts);
    assert(bpr.err == GEOQIK_SUCCESS);

    for (std::size_t i = 0; i < batchSize; ++i) {
        batchPts[i * 3 + 1] = 1.2;
    }
    geoqik_update_points_options_t ubOpts{};
    ubOpts.color      = batchColors.data();
    ubOpts.colorCount = batchColors.size();
    err = geoqik_update_points_opts(&bpr.geometryId, batchPts.data(), batchPts.size(), &ubOpts);
    assert(err == GEOQIK_SUCCESS);

    err = geoqik_remove_point(&pr.geometryId);
    assert(err == GEOQIK_SUCCESS);

    err = geoqik_add_line_with_color(0.0, 0.0, 0.0, 1.0, 1.0, 0.0,
        1.0f, 1.0f, 0.0f, 1.0f);
    assert(err == GEOQIK_SUCCESS);

    geoqik_uuid_t lineKey{};
    (void)geoqik_generate_uuid(&lineKey);
    geoqik_add_line_opts_t lOpts{};
    lOpts.idempotencyKey = lineKey;
    lOpts.color          = green;
    lOpts.colorCount     = 4;
    auto lr = geoqik_add_line_opts(0.0, 0.0, 0.0, 2.0, 0.0, 0.0, &lOpts);
    assert(lr.err == GEOQIK_SUCCESS);

    err = geoqik_update_line(&lr.geometryId, 0.0, 0.0, 0.0, 2.0, 1.0, 0.0);
    assert(err == GEOQIK_SUCCESS);

    err = geoqik_update_line_with_color(&lr.geometryId,
        0.0, 0.0, 0.0, 2.0, 2.0, 0.0,
        0.0f, 0.0f, 1.0f, 1.0f);
    assert(err == GEOQIK_SUCCESS);

    constexpr std::size_t lineCount = 3;
    std::array<double, lineCount * 6> linesData{
        0.0, 0.0, 0.0,  1.0, 0.0, 0.0,
        0.0, 0.0, 0.0,  0.0, 1.0, 0.0,
        0.0, 0.0, 0.0,  0.0, 0.0, 1.0
    };
    geoqik_add_line_opts_t batchLineOpts{};
    batchLineOpts.color      = red;
    batchLineOpts.colorCount = 4;
    [[maybe_unused]] auto blr = geoqik_add_lines_opts(linesData.data(), linesData.size(), &batchLineOpts);
    assert(blr.err == GEOQIK_SUCCESS);

    err = geoqik_remove_line(&lr.geometryId);
    assert(err == GEOQIK_SUCCESS);

    err = geoqik_replay_current_log(NULL);
    assert(err == GEOQIK_SUCCESS);

    err = geoqik_wait_for_exit_and_cleanup();
    assert(err == GEOQIK_SUCCESS);
    return 0;
}
