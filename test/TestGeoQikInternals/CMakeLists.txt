function(create_geoqik_internal_test testName)
    add_executable(${testName} ${testName}.cpp)
    target_link_libraries(${testName} PRIVATE gtest gtest_main geoqik::geoqik_internals)
    add_test(NAME geoqik.${testName} COMMAND ${testName})
    set_property(GLOBAL APPEND PROPERTY ${PROJECT_NAME}_COVERAGE_TESTS ${testName})
endfunction()

function(create_geoqik_opengl_internal_test testName)
    add_executable(${testName} ${testName}.cpp)
    target_link_libraries(${testName} PRIVATE
            gtest
            gtest_main
            renderer::opengl
            geoqik::core
            glad::glad
            glfw
            linal::linal)
    add_test(NAME geoqik.${testName} COMMAND ${testName})
    set_tests_properties(geoqik.${testName} PROPERTIES RESOURCE_LOCK geoqik_glfw)
    set_property(GLOBAL APPEND PROPERTY ${PROJECT_NAME}_COVERAGE_TESTS ${testName})
endfunction()

function(create_renderer_test testName)
    add_executable(${testName} ${testName}.cpp)
    target_link_libraries(${testName} PRIVATE
            gtest
            gtest_main
            renderer::renderer)
    add_test(NAME geoqik.${testName} COMMAND ${testName})
    set_property(GLOBAL APPEND PROPERTY ${PROJECT_NAME}_COVERAGE_TESTS ${testName})
endfunction()

function(create_geoqik_opengl_program_unit_test testName)
    add_executable(${testName}
            ${testName}.cpp
            ${CMAKE_SOURCE_DIR}/src/renderer/opengl/source/Programs/CreateProgram.cpp
            ${CMAKE_SOURCE_DIR}/src/renderer/opengl/source/Programs/ProgramId.cpp)
    target_include_directories(${testName} PRIVATE
            ${CMAKE_BINARY_DIR}/src/renderer/opengl/export_header_includes
            ${CMAKE_SOURCE_DIR}/src/renderer/opengl/include
            ${CMAKE_SOURCE_DIR}/src/renderer/include
            ${CMAKE_SOURCE_DIR}/src/renderer/opengl/source)
    target_compile_definitions(${testName} PRIVATE OPENGL_STATIC_DEFINE)
    target_compile_features(${testName} PRIVATE cxx_std_23)
    target_link_libraries(${testName} PRIVATE
            gmock
            gtest_main
            geoqik::core
            fmt::fmt
            glad::glad)
    add_test(NAME geoqik.${testName} COMMAND ${testName})
    set_property(GLOBAL APPEND PROPERTY ${PROJECT_NAME}_COVERAGE_TESTS ${testName})
endfunction()

create_geoqik_internal_test(AssertTest)
create_geoqik_internal_test(BufferTest)
create_renderer_test(CameraAutoFitTest)
create_renderer_test(CameraInteractorTest)
create_renderer_test(CameraTest)
create_geoqik_internal_test(ConcurrentQueueTest)
create_geoqik_opengl_internal_test(DrawableTest)
create_geoqik_internal_test(GeometryBufferTest)
create_geoqik_internal_test(MeshBufferTest)
create_renderer_test(InputCaptureStateTest)
create_renderer_test(InputStateTest)
set_tests_properties(geoqik.InputStateTest PROPERTIES RESOURCE_LOCK geoqik_glfw)
create_geoqik_internal_test(WindowSettingsTest)
target_link_libraries(WindowSettingsTest PRIVATE geoqik::geoqik)
create_geoqik_opengl_program_unit_test(CreateProgramTest)
create_renderer_test(RayPlaneIntersectionTest)
create_geoqik_internal_test(SceneTest)
create_geoqik_internal_test(TransparencySortingTest)
create_geoqik_internal_test(TestGeoQikLog)
create_geoqik_internal_test(TestGeoQikReplayUndo)
