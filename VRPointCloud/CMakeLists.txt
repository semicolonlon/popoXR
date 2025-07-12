cmake_minimum_required(VERSION 3.22.1)
project(VRPointCloud)

# Additional Directories for find_package() to search within.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")
include("../cmake/graphics_api_select.cmake")

include(FetchContent)

# PCL
find_package(PCL 1.13 REQUIRED)
include_directories(${PCL_INCLUDE_DIRS})
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})

# Fetch OpenXR SDK
set(BUILD_TESTS OFF CACHE INTERNAL "Build tests")
set(BUILD_API_LAYERS ON CACHE INTERNAL "Use OpenXR layers")
FetchContent_Declare(
    OpenXR
    URL https://github.com/KhronosGroup/OpenXR-SDK-Source/archive/refs/tags/release-1.0.34.zip
    URL_HASH MD5=924a94a2da0b5ef8e82154c623d88644
)
FetchContent_MakeAvailable(OpenXR)

# Sources and headers
set(SOURCE_FILES
    main.cpp
    OpenXR.h
    OpenXR.cpp
    OpenXRManager.cpp
    ../External/GraphicsAPI.cpp
    ../External/GraphicsAPI_OpenGL.cpp
    ../External/OpenXRDebugUtils.cpp
)
set(HEADER_FILES
    ../External/DebugOutput.h
    ../External/GraphicsAPI.h
    ../External/GraphicsAPI_OpenGL.h
    ../External/HelperFunctions.h
    ../External/OpenXRDebugUtils.h
    ../External/OpenXRHelper.h
)

# GLSL Shaders
set(GLSL_SHADERS
    ../Shaders/VertexShader.glsl
    ../Shaders/FragmentShader.glsl
)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
    PRIVATE
        ${SOURCE_FILES}
        ${HEADER_FILES}
 "OpenXR.h")

# Shader copy
set(SHADER_DEST "${CMAKE_CURRENT_BINARY_DIR}")
foreach(SHADER ${GLSL_SHADERS})
    get_filename_component(FILE_WE ${SHADER} NAME_WE)
    set(SHADER_OUTPUT "${SHADER_DEST}/${FILE_WE}.glsl")

    add_custom_command(
        OUTPUT ${SHADER_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}" "${SHADER_OUTPUT}"
        MAIN_DEPENDENCY ${SHADER}
        COMMENT "Copying GLSL shader: ${SHADER}"
        VERBATIM
    )
    target_sources(${PROJECT_NAME} PRIVATE ${SHADER_OUTPUT})
endforeach()

# Environment for VS Debugger
if(NOT "${XR_RUNTIME_JSON}" STREQUAL "")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_DEBUGGER_ENVIRONMENT "XR_RUNTIME_JSON=${XR_RUNTIME_JSON}"
    )
endif()

# Includes
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ../External/
        "${openxr_SOURCE_DIR}/src/Common"
        "${openxr_SOURCE_DIR}/external/include"
        ${PCL_LIBRARIES}
)

# Link OpenXR loader
target_link_libraries(${PROJECT_NAME} openxr_loader)

# Add OpenGL API define
addgraphicsapidefine(${PROJECT_NAME})

# OpenGL wrapper
include(../cmake/gfxwrapper.cmake)
if(TARGET openxr-gfxwrapper)
    target_link_libraries(${PROJECT_NAME} openxr-gfxwrapper)
    target_compile_definitions(${PROJECT_NAME} PUBLIC XR_TUTORIAL_USE_OPENGL)
endif()
