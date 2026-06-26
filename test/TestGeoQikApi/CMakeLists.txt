function(create_api_test testName)
    add_executable(${testName} ${testName}.cpp)
    target_link_libraries(${testName} PRIVATE gtest_main geoqik::geoqik)
    add_test(NAME geoqik.${testName} COMMAND ${testName})
    set_tests_properties(geoqik.${testName} PROPERTIES
        RESOURCE_LOCK geoqik_glfw
        TIMEOUT 60)
    set_property(GLOBAL APPEND PROPERTY ${PROJECT_NAME}_COVERAGE_TESTS ${testName})
endfunction()

create_api_test(TestGeoQikApi)
create_api_test(TestGeoQikApi_Points)
create_api_test(TestGeoQikApi_Lines)
