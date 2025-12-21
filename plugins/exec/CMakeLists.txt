cmake_minimum_required(VERSION 3.10)

project(pchat-exec C)

# This plugin is Windows-only
if(NOT WIN32)
    message(STATUS "Exec plugin is Windows-only, skipping")
    return()
endif()

# Plugin source files
set(PLUGIN_SOURCES
    exec.c
)

# Create shared library
add_library(exec MODULE ${PLUGIN_SOURCES})

# Set output name and properties
set_target_properties(exec PROPERTIES
    PREFIX ""
    OUTPUT_NAME "pchatexec"
    SUFFIX ".dll"
)

# Include directories
target_include_directories(exec PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
)

# Compiler definitions
target_compile_definitions(exec PRIVATE
    WIN32_LEAN_AND_MEAN
)

# Compiler flags
target_compile_options(exec PRIVATE
    -Wall
    -Wextra
)

# Install plugin
install(TARGETS exec
    LIBRARY DESTINATION lib/pchat/plugins
    RUNTIME DESTINATION lib/pchat/plugins
    COMPONENT Plugins
)
