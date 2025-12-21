# Translations
file(GLOB PO_FILES *.po)

foreach(PO_FILE ${PO_FILES})
    get_filename_component(LANG ${PO_FILE} NAME_WE)
    set(MO_FILE ${CMAKE_CURRENT_BINARY_DIR}/${LANG}.mo)
    
    add_custom_command(
        OUTPUT ${MO_FILE}
        COMMAND ${GETTEXT_MSGFMT_EXECUTABLE} -o ${MO_FILE} ${PO_FILE}
        DEPENDS ${PO_FILE}
    )
    
    install(FILES ${MO_FILE}
        DESTINATION ${CMAKE_INSTALL_LOCALEDIR}/${LANG}/LC_MESSAGES
        RENAME pchat.mo
        COMPONENT Translations
    )
    
    list(APPEND MO_FILES ${MO_FILE})
endforeach()

add_custom_target(translations ALL DEPENDS ${MO_FILES})
