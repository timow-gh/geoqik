#include <plinth/WindowSettings.hpp>

#include <GeoQik/GeoQik.hpp>
#include "GeoQikSettings.hpp"
#include <gtest/gtest.h>

#include <cstddef>

using namespace renderer;

TEST(WindowSettingsTest, ExternalApiUsesGeoQikDefaultTitle) {
    geoqik_window_settings_t externalSettings;
    geoqik_init_default_window_settings(&externalSettings);

    const WindowSettings internalSettings;

    EXPECT_STREQ(externalSettings.title, "GeoQik Viewer");
    EXPECT_EQ(externalSettings.width, internalSettings.width);
    EXPECT_EQ(externalSettings.height, internalSettings.height);
    EXPECT_EQ(externalSettings.red_bits, internalSettings.red_bits);
    EXPECT_EQ(externalSettings.green_bits, internalSettings.green_bits);
    EXPECT_EQ(externalSettings.blue_bits, internalSettings.blue_bits);
    EXPECT_EQ(externalSettings.alpha_bits, internalSettings.alpha_bits);
    EXPECT_EQ(externalSettings.depth_bits, internalSettings.depth_bits);
    EXPECT_EQ(externalSettings.stencil_bits, internalSettings.stencil_bits);
    EXPECT_EQ(externalSettings.accum_red_bits, internalSettings.accum_red_bits);
    EXPECT_EQ(externalSettings.accum_green_bits, internalSettings.accum_green_bits);
    EXPECT_EQ(externalSettings.accum_blue_bits, internalSettings.accum_blue_bits);
    EXPECT_EQ(externalSettings.accum_alpha_bits, internalSettings.accum_alpha_bits);
    EXPECT_EQ(externalSettings.aux_buffers, internalSettings.aux_buffers);
    EXPECT_EQ(externalSettings.samples, internalSettings.samples);
    EXPECT_EQ(externalSettings.refresh_rate, internalSettings.refresh_rate);
    EXPECT_EQ(externalSettings.stereo != 0, internalSettings.stereo);
    EXPECT_EQ(externalSettings.srgb_capable != 0, internalSettings.srgb_capable);
    EXPECT_EQ(externalSettings.double_buffer != 0, internalSettings.double_buffer);
    EXPECT_EQ(externalSettings.resizable != 0, internalSettings.resizable);
    EXPECT_EQ(externalSettings.visible != 0, internalSettings.visible);
    EXPECT_EQ(externalSettings.decorated != 0, internalSettings.decorated);
    EXPECT_EQ(externalSettings.focused != 0, internalSettings.focused);
    EXPECT_EQ(externalSettings.auto_iconify != 0, internalSettings.auto_iconify);
    EXPECT_EQ(externalSettings.floating != 0, internalSettings.floating);
    EXPECT_EQ(externalSettings.maximized != 0, internalSettings.maximized);
    EXPECT_EQ(externalSettings.center_cursor != 0, internalSettings.center_cursor);
    EXPECT_EQ(externalSettings.transparent_framebuffer != 0, internalSettings.transparent_framebuffer);
    EXPECT_EQ(externalSettings.focus_on_show != 0, internalSettings.focus_on_show);
    EXPECT_EQ(externalSettings.scale_to_monitor != 0, internalSettings.scale_to_monitor);
}

TEST(GeoQikSettingsTest, CAndCppDefaultsMatchPlinthAppearance) {
    geoqik_settings_t cSettings{};
    geoqik_create_default_settings(&cSettings);
    const geoqik::GeoQikSettings cppSettings;

    EXPECT_EQ(cSettings.backgroundColor[0], 0.05F);
    EXPECT_EQ(cSettings.backgroundColor[1], 0.05F);
    EXPECT_EQ(cSettings.backgroundColor[2], 0.05F);
    EXPECT_EQ(cSettings.backgroundColor[3], 1.0F);
    EXPECT_EQ(cSettings.meshFillLightDirection[0], -0.5F);
    EXPECT_EQ(cSettings.meshFillLightDirection[1], -0.4F);
    EXPECT_EQ(cSettings.meshFillLightDirection[2], 0.6F);
    EXPECT_EQ(cSettings.meshFillLightColor[0], 0.25F);
    EXPECT_EQ(cSettings.meshFillLightColor[1], 0.28F);
    EXPECT_EQ(cSettings.meshFillLightColor[2], 0.35F);
    EXPECT_EQ(cSettings.meshFillLightIntensity, 1.0F);
    EXPECT_EQ(cSettings.meshAmbientIntensity, 0.30F);
    EXPECT_EQ(cSettings.meshShininess, 1.0F);

    EXPECT_EQ(cSettings.backgroundColor[0], cppSettings.backgroundColor[0]);
    EXPECT_EQ(cSettings.backgroundColor[1], cppSettings.backgroundColor[1]);
    EXPECT_EQ(cSettings.backgroundColor[2], cppSettings.backgroundColor[2]);
    EXPECT_EQ(cSettings.backgroundColor[3], cppSettings.backgroundColor[3]);
    for (std::size_t channel = 0; channel < 3; ++channel) {
        EXPECT_EQ(cSettings.meshFillLightDirection[channel], cppSettings.meshFillLightDirection[channel]);
        EXPECT_EQ(cSettings.meshFillLightColor[channel], cppSettings.meshFillLightColor[channel]);
    }
    EXPECT_EQ(cSettings.meshFillLightIntensity, cppSettings.meshFillLightIntensity);
    EXPECT_EQ(cSettings.meshAmbientIntensity, cppSettings.meshAmbientIntensity);
    EXPECT_EQ(cSettings.meshShininess, cppSettings.meshShininess);
}
