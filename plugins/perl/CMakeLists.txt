cmake_minimum_required(VERSION 3.10)

project(pchat-perl C)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Perl)

if(NOT PERL_FOUND)
    message(STATUS "Perl not found, skipping Perl plugin")
    return()
endif()

message(STATUS "Found Perl: ${PERL_EXECUTABLE}")

# Get Perl compilation flags
execute_process(
    COMMAND ${PERL_EXECUTABLE} -MExtUtils::Embed -e ccopts
    OUTPUT_VARIABLE PERL_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

execute_process(
    COMMAND ${PERL_EXECUTABLE} -MExtUtils::Embed -e ldopts
    OUTPUT_VARIABLE PERL_LDFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# Clean up flags
string(STRIP "${PERL_CFLAGS}" PERL_CFLAGS)
string(STRIP "${PERL_LDFLAGS}" PERL_LDFLAGS)

# Convert flags to lists
separate_arguments(PERL_CFLAGS_LIST UNIX_COMMAND "${PERL_CFLAGS}")
separate_arguments(PERL_LDFLAGS_LIST UNIX_COMMAND "${PERL_LDFLAGS}")

pkg_check_modules(GLIB REQUIRED glib-2.0)

# Generate headers from Perl modules
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xchat.pm.h ${CMAKE_CURRENT_BINARY_DIR}/irc.pm.h
    COMMAND ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_header
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lib/XChat.pm
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/XChat/Embed.pm
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/XChat/List/Network.pm
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/XChat/List/Network/Entry.pm
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/XChat/List/Network/AutoJoin.pm
            ${CMAKE_CURRENT_SOURCE_DIR}/lib/IRC.pm
    COMMENT "Generating Perl module headers"
)

# Plugin source files
set(PLUGIN_SOURCES
    perl.c
    ${CMAKE_CURRENT_BINARY_DIR}/xchat.pm.h
    ${CMAKE_CURRENT_BINARY_DIR}/irc.pm.h
)

# Create shared library
add_library(perl MODULE ${PLUGIN_SOURCES})

# Set output name and properties
set_target_properties(perl PROPERTIES
    PREFIX ""
    OUTPUT_NAME "perl"
)

# Set appropriate suffix based on platform
if(WIN32)
    set_target_properties(perl PROPERTIES SUFFIX ".dll")
else()
    set_target_properties(perl PROPERTIES SUFFIX ".so")
endif()

# Include directories
target_include_directories(perl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src/common
    ${GLIB_INCLUDE_DIRS}
)

# Add Perl include paths
target_compile_options(perl PRIVATE
    ${PERL_CFLAGS_LIST}
    ${GLIB_CFLAGS_OTHER}
    -Wall
    -Wno-unused
)

# Compiler definitions
target_compile_definitions(perl PRIVATE
    HAVE_CONFIG_H
)

# Link libraries
target_link_libraries(perl
    ${GLIB_LIBRARIES}
)

# Add Perl linker flags
target_link_options(perl PRIVATE
    ${PERL_LDFLAGS_LIST}
)

# Platform-specific link options
if(UNIX AND NOT APPLE)
    target_link_options(perl PRIVATE 
        "-Wl,--unresolved-symbols=ignore-all"
    )
endif()

# Install plugin
install(TARGETS perl
    LIBRARY DESTINATION lib/pchat/plugins
    RUNTIME DESTINATION lib/pchat/plugins
)
