include(FetchContent)

if (${PROJECT_NAME}_TESTS)
  add_subdirectory(googletest)
endif ()

add_subdirectory(fmt)
add_subdirectory(linal)
add_subdirectory(plinth)
add_subdirectory(nlohmann_json)
