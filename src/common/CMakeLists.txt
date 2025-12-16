# Common library
add_library(pchatcommon STATIC
    cfgfiles.c
    chanopt.c
    ctcp.c
    dcc.c
    xchat.c
    history.c
    ignore.c
    inbound.c
    marshal.c
    modes.c
    network.c
    notify.c
    outbound.c
    plugin.c
    plugin-timer.c
    proto-irc.c
    server.c
    servlist.c
    text.c
    tree.c
    url.c
    userlist.c
    util.c
)

# Platform-specific sources
if(WIN32)
    target_sources(pchatcommon PRIVATE
        identd.c
        pchat-platform-support-win32.c
    )
else()
    target_sources(pchatcommon PRIVATE
        pchat-platform-support.c
    )
endif()

# OpenSSL support
if(ENABLE_OPENSSL)
    target_sources(pchatcommon PRIVATE ssl.c)
    target_include_directories(pchatcommon PRIVATE ${OPENSSL_INCLUDE_DIRS})
    target_link_libraries(pchatcommon PRIVATE ${OPENSSL_LIBRARIES})
    target_link_directories(pchatcommon PRIVATE ${OPENSSL_LIBRARY_DIRS})
endif()

# Compile definitions
target_compile_definitions(pchatcommon PRIVATE HAVE_CONFIG_H)

# Include directories
target_include_directories(pchatcommon PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_include_directories(pchatcommon PRIVATE
    ${GLIB_INCLUDE_DIRS}
)

target_link_libraries(pchatcommon PRIVATE
    ${GLIB_LIBRARIES}
)

# Windows socket library
if(WIN32)
    target_link_libraries(pchatcommon PRIVATE ws2_32)
endif()

if(LIBPROXY_FOUND)
    target_include_directories(pchatcommon PRIVATE ${LIBPROXY_INCLUDE_DIRS})
    target_link_libraries(pchatcommon PRIVATE ${LIBPROXY_LIBRARIES})
endif()

# Generate text events
add_executable(make-te make-te.c)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/textevents.h ${CMAKE_CURRENT_BINARY_DIR}/textenums.h
    COMMAND make-te < ${CMAKE_CURRENT_SOURCE_DIR}/textevents.in > ${CMAKE_CURRENT_BINARY_DIR}/textevents.h 2> ${CMAKE_CURRENT_BINARY_DIR}/textenums.h
    DEPENDS make-te ${CMAKE_CURRENT_SOURCE_DIR}/textevents.in
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Generate marshal files
find_program(GLIB_GENMARSHAL glib-genmarshal REQUIRED)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/marshal.h
    COMMAND ${GLIB_GENMARSHAL} --prefix=_pchat_marshal --header ${CMAKE_CURRENT_SOURCE_DIR}/marshalers.list > ${CMAKE_CURRENT_BINARY_DIR}/marshal.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/marshalers.list
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/marshal.c
    COMMAND ${GLIB_GENMARSHAL} --prefix=_pchat_marshal --body ${CMAKE_CURRENT_SOURCE_DIR}/marshalers.list > ${CMAKE_CURRENT_BINARY_DIR}/marshal.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/marshalers.list
)

# Add generated files as dependencies
target_sources(pchatcommon PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/textevents.h
    ${CMAKE_CURRENT_BINARY_DIR}/textenums.h
    ${CMAKE_CURRENT_BINARY_DIR}/marshal.h
    ${CMAKE_CURRENT_BINARY_DIR}/marshal.c
)

# D-Bus support
if(USE_DBUS)
    add_subdirectory(dbus)
    target_link_libraries(pchatcommon PRIVATE pchat-dbus)
endif()

# Install headers if plugin support is enabled
if(ENABLE_PLUGIN)
    install(FILES xchat-plugin.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/pchat)
endif()
