#include <GeoQikClient/GeoQikClient.hpp>
#include <cassert>
#include <cmath>
#include <iostream>

int
main()
{
    geoqik_settings_t settings{};
    geoqik_create_default_settings(&settings);
    assert(settings.defaultPointSize > 0.0f);
    assert(settings.defaultLineWidth > 0.0f);

    geoqik_window_settings_t windowSettings{};
    geoqik_init_default_window_settings(&windowSettings);
    assert(windowSettings.width == 1280);
    assert(windowSettings.height == 720);

    assert(geoqik_get_error_string(GEOQIK_SUCCESS) != nullptr);
    assert(geoqik_get_error_string(GEOQIK_ERROR_NOT_INITIALIZED) != nullptr);

    geoqik_uuid_t uuid{};
    [[maybe_unused]]
    const auto uuidErr = geoqik_generate_uuid(&uuid);
    assert(uuidErr == GEOQIK_SUCCESS);

    settings.defaultPointSize = 8.0f;
    windowSettings.width = 800;
    windowSettings.height = 600;
    windowSettings.title = "GeoQik Settings Test";

    [[maybe_unused]]
    auto err = geoqik_init_with_settings(&settings, &windowSettings);
    assert(err == GEOQIK_SUCCESS && "geoqik_init_with_settings failed; is GEOQIK_EXE_PATH set?");

    bool initialized = false;
    err = geoqik_is_api_initialized(&initialized);
    assert(err == GEOQIK_SUCCESS);
    assert(initialized);

    err = geoqik_set_point_size(6.0f);
    assert(err == GEOQIK_SUCCESS);

    float readBack = 0.0f;
    err = geoqik_get_point_size(&readBack);
    assert(err == GEOQIK_SUCCESS);
    assert(std::abs(readBack - 6.0f) < 1e-4f);

    err = geoqik_set_point_color(1.0f, 0.0f, 0.0f, 1.0f);
    assert(err == GEOQIK_SUCCESS);

    float r = 0, g = 0, b = 0, a = 0;
    err = geoqik_get_point_color(&r, &g, &b, &a);
    assert(err == GEOQIK_SUCCESS);
    assert(std::abs(r - 1.0f) < 1e-4f && std::abs(g) < 1e-4f);

    err = geoqik_set_line_width(3.0f);
    assert(err == GEOQIK_SUCCESS);

    err = geoqik_set_line_color(0.0f, 1.0f, 0.0f, 1.0f);
    assert(err == GEOQIK_SUCCESS);

    err = geoqik_set_mesh_color(0.0f, 0.0f, 1.0f, 1.0f);
    assert(err == GEOQIK_SUCCESS);

    err = geoqik_draw();
    assert(err == GEOQIK_SUCCESS);

    auto pr = geoqik_add_point(1.0, 0.0, 0.0);
    assert(pr.err == GEOQIK_SUCCESS);

    err = geoqik_translate_geometry(&pr.geometryId, 0.5, 0.5, 0.0);
    assert(err == GEOQIK_SUCCESS);

    err = geoqik_rotate_geometry(&pr.geometryId, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 3.14159265358979323846 / 4.0);
    assert(err == GEOQIK_SUCCESS);

    for (int i = 0; i < 20; ++i) {
        [[maybe_unused]]
        const auto result = geoqik_add_point(static_cast<double>(i) * 0.1, 0.0, 0.0);
        assert(result.err == GEOQIK_SUCCESS);
    }

    err = geoqik_remove_all_geometry();
    assert(err == GEOQIK_SUCCESS);

    [[maybe_unused]]
    const auto pointA = geoqik_add_point(0.0, 1.0, 0.0);
    assert(pointA.err == GEOQIK_SUCCESS);
    [[maybe_unused]]
    const auto pointB = geoqik_add_point(1.0, 0.0, 0.0);
    assert(pointB.err == GEOQIK_SUCCESS);

    err = geoqik_wait_for_exit_and_cleanup();
    assert(err == GEOQIK_SUCCESS);

    return 0;
}
