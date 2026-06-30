add_executable(TestGeoQikClient TestGeoQikClient.cpp)

target_link_libraries(TestGeoQikClient PRIVATE
    gtest_main
    geoqik::client)

# Pass the server executable path so ProcessManager can find it without PATH manipulation.
target_compile_definitions(TestGeoQikClient PRIVATE
    "GEOQIK_SERVER_EXE_PATH=\"$<TARGET_FILE:geoqik::server>\"")

add_dependencies(TestGeoQikClient geoqik_server)

add_test(NAME geoqik.TestGeoQikClient COMMAND TestGeoQikClient)
set_tests_properties(geoqik.TestGeoQikClient PROPERTIES
    TIMEOUT 90)
set_property(GLOBAL APPEND PROPERTY ${PROJECT_NAME}_COVERAGE_TESTS TestGeoQikClient)
