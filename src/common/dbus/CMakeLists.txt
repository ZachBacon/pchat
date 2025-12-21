# D-Bus support library

# Generate remote-object-glue.h from remote-object.xml
find_program(DBUS_BINDING_TOOL dbus-binding-tool)
if(DBUS_BINDING_TOOL)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/remote-object-glue.h
        COMMAND ${DBUS_BINDING_TOOL}
            --prefix=remote_object
            --mode=glib-server
            --output=${CMAKE_CURRENT_BINARY_DIR}/remote-object-glue.h
            ${CMAKE_CURRENT_SOURCE_DIR}/remote-object.xml
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/remote-object.xml
        COMMENT "Generating D-Bus glue code"
    )
else()
    message(WARNING "dbus-binding-tool not found, D-Bus glue code will not be generated")
endif()

# D-Bus library
add_library(pchat-dbus STATIC
    dbus-plugin.c
    dbus-plugin.h
    dbus-client.c
    dbus-client.h
    ${CMAKE_CURRENT_BINARY_DIR}/remote-object-glue.h
)

target_include_directories(pchat-dbus
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/common
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/src/common
        ${DBUS_INCLUDE_DIRS}
        ${GLIB_INCLUDE_DIRS}
)

target_link_libraries(pchat-dbus
    PRIVATE
        ${DBUS_LIBRARIES}
        ${GLIB_LIBRARIES}
)

# Example program
if(DBUS_BINDING_TOOL)
    add_executable(dbus-example EXCLUDE_FROM_ALL
        example.c
    )
    
    target_include_directories(dbus-example
        PRIVATE
            ${DBUS_INCLUDE_DIRS}
            ${GLIB_INCLUDE_DIRS}
    )
    
    target_link_libraries(dbus-example
        PRIVATE
            ${DBUS_LIBRARIES}
            ${GLIB_LIBRARIES}
    )
endif()

# Generate D-Bus service file
set(DBUS_SERVICES_DIR "${CMAKE_INSTALL_PREFIX}/share/dbus-1/services" CACHE PATH "D-Bus services directory")
set(bindir "${CMAKE_INSTALL_FULL_BINDIR}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/org.pchat.service.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/org.pchat.service.service
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.pchat.service.service
    DESTINATION ${DBUS_SERVICES_DIR}
)
