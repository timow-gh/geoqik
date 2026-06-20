set(renderer_HEADER_FILES
        include/Renderer/RendererGlfw.hpp
        include/Renderer/WindowSettings.hpp
        include/Renderer/InputState.hpp
        include/Renderer/InputCaptureState.hpp
        include/Renderer/GlfwWindow.hpp
        include/Renderer/CameraProjectionType.hpp
        include/Renderer/MathConstants.hpp
        include/Renderer/PickRay.hpp
        include/Renderer/Color.hpp
        include/Renderer/Viewport.hpp
        include/Renderer/Camera.hpp
        include/Renderer/CameraAutoFit.hpp
        include/Renderer/RayPlaneIntersection.hpp
        include/Renderer/CameraInteractor.hpp
        include/Renderer/ImGuiOverlay.hpp
        include/Renderer/Renderer.hpp
)

set(renderer_SOURCE_FILES
        source/InputState.cpp
        source/GlfwWindow.cpp
        source/Camera.cpp
        source/CameraAutoFit.cpp
        source/ImGuiOverlay.cpp
        source/Renderer.cpp
)

set(TARGET_NAME renderer_renderer)
add_library(${TARGET_NAME} STATIC ${renderer_SOURCE_FILES} ${renderer_HEADER_FILES})
set_target_properties(${TARGET_NAME}
        PROPERTIES
        LINKER_LANGUAGE CXX
        POSITION_INDEPENDENT_CODE ON)

include(GenerateExportHeader)
set(EXPORT_HEADER_BASE_NAME "renderer")
generate_export_header(${TARGET_NAME} BASE_NAME ${EXPORT_HEADER_BASE_NAME})
string(TOLOWER "${EXPORT_HEADER_BASE_NAME}_export.h" GENERATED_TARGET_EXPORTS_FILE_NAME)
file(COPY
        ${CMAKE_CURRENT_BINARY_DIR}/${GENERATED_TARGET_EXPORTS_FILE_NAME}
        DESTINATION
        ${CMAKE_CURRENT_BINARY_DIR}/export_header_includes/Renderer)
file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/${GENERATED_TARGET_EXPORTS_FILE_NAME})

target_include_directories(${TARGET_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/export_header_includes>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/opengl/include>
        $<INSTALL_INTERFACE:include>)

target_compile_definitions(${TARGET_NAME} PUBLIC RENDERER_STATIC_DEFINE)
target_compile_features(${TARGET_NAME} PUBLIC cxx_std_23)

add_library(renderer::renderer ALIAS ${TARGET_NAME})

target_link_libraries(${TARGET_NAME}
        PUBLIC
        renderer::opengl
        glad::glad
        glfw
        $<BUILD_LOCAL_INTERFACE:linal::linal>
        Geometry::Geometry
        PRIVATE
        imgui::imgui
)

add_warnings_and_compile_options(${TARGET_NAME} "${${PROJECT_NAME}_WARNINGS_AS_ERRORS}")
if (${PROJECT_NAME}_STATIC_ANALYSIS)
    enable_static_analysis(${TARGET_NAME} "${${PROJECT_NAME}_WARNINGS_AS_ERRORS}")
endif ()

add_subdirectory(opengl)
