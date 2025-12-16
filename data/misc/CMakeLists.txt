# Install desktop file and appdata
configure_file(pchat.appdata.xml.in pchat.appdata.xml @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pchat.appdata.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES pchat.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
