#pragma once

#include <GeoQik/GeoQik.hpp>
#include <gtest/gtest.h>

class GeoQikApiTestBase : public ::testing::Test
{
protected:
  static geoqik_error_code_t init_hidden_geoqik(const geoqik_settings_t* settings = nullptr)
  {
    geoqik_window_settings_t windowSettings;
    geoqik_init_default_window_settings(&windowSettings);
    windowSettings.visible = 0;

    if (settings != nullptr)
    {
      return geoqik_init_with_settings(settings, &windowSettings);
    }

    geoqik_settings_t defaultSettings;
    geoqik_create_default_settings(&defaultSettings);
    return geoqik_init_with_settings(&defaultSettings, &windowSettings);
  }
};
