cmake_minimum_required(VERSION 3.10)

project(pchat-upd C)

# This plugin is Windows-only
if(NOT WIN32)
    message(STATUS "Update Checker plugin is Windows-only, skipping")
    return()
endif()

# Try to find WinSparkle
find_library(WINSPARKLE_LIBRARY
    NAMES WinSparkle
    PATHS
        ${CMAKE_PREFIX_PATH}/lib
        ${CMAKE_SYSTEM_PREFIX_PATH}/lib
)

find_path(WINSPARKLE_INCLUDE_DIR
    NAMES winsparkle.h
    PATHS
        ${CMAKE_PREFIX_PATH}/include
        ${CMAKE_SYSTEM_PREFIX_PATH}/include
)

if(NOT WINSPARKLE_LIBRARY OR NOT WINSPARKLE_INCLUDE_DIR)
    message(STATUS "WinSparkle not found, skipping Update Checker plugin")
    return()
endif()

message(STATUS "Found WinSparkle: ${WINSPARKLE_LIBRARY}")

# Plugin source files
set(PLUGIN_SOURCES
    upd.c
)

# Create shared library
add_library(upd MODULE ${PLUGIN_SOURCES})

# Set output name and properties
set_target_properties(upd PROPERTIES
    PREFIX ""
    OUTPUT_NAME "pchatupd"
    SUFFIX ".dll"
)

# Include directories
target_include_directories(upd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
    ${WINSPARKLE_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(upd
    ${WINSPARKLE_LIBRARY}
)

# Compiler definitions
target_compile_definitions(upd PRIVATE
    WIN32_LEAN_AND_MEAN
)

# Compiler flags
target_compile_options(upd PRIVATE
    -Wall
    -Wextra
)

# Install plugin
install(TARGETS upd
    LIBRARY DESTINATION lib/pchat/plugins
    RUNTIME DESTINATION lib/pchat/plugins
    COMPONENT Plugins
)
