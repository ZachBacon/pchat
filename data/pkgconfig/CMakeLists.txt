# Generate and install pkg-config file
configure_file(pchat-plugin.pc.in pchat-plugin.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pchat-plugin.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig COMPONENT Headers)
