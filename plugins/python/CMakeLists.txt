cmake_minimum_required(VERSION 3.10)

project(pchat-python C)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Python3 COMPONENTS Interpreter Development)

if(NOT Python3_FOUND)
    message(STATUS "Python3 not found, skipping Python plugin")
    return()
endif()

message(STATUS "Found Python: ${Python3_VERSION} (${Python3_EXECUTABLE})")

pkg_check_modules(GLIB REQUIRED glib-2.0)

# Plugin source files
set(PLUGIN_SOURCES
    python.c
)

# Add .def file on Windows to export symbols
if(WIN32)
    list(APPEND PLUGIN_SOURCES python.def)
endif()

# Create shared library
add_library(python MODULE ${PLUGIN_SOURCES})

# Set output name and properties
set_target_properties(python PROPERTIES
    PREFIX ""
    OUTPUT_NAME "python"
)

# Set appropriate suffix based on platform
if(WIN32)
    set_target_properties(python PROPERTIES SUFFIX ".dll")
else()
    set_target_properties(python PROPERTIES SUFFIX ".so")
endif()

# Include directories
target_include_directories(python PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_BINARY_DIR}
    ${Python3_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(python
    ${Python3_LIBRARIES}
    ${GLIB_LIBRARIES}
)

# Compiler definitions
target_compile_definitions(python PRIVATE
    HAVE_CONFIG_H
)

# Compiler flags
target_compile_options(python PRIVATE
    -Wall
    -Wextra
    ${GLIB_CFLAGS_OTHER}
)

# Platform-specific link options
if(UNIX AND NOT APPLE)
    target_link_options(python PRIVATE 
        "-Wl,--unresolved-symbols=ignore-all"
    )
endif()

# Install plugin
install(TARGETS python
    LIBRARY DESTINATION lib/pchat/plugins
    RUNTIME DESTINATION lib/pchat/plugins
)

# Install Python support files
install(FILES
    pchat.py
    python.py
    xchat.py
    _pchat.py
    DESTINATION lib/pchat/plugins
)
