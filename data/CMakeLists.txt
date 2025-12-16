# Data files installation
add_subdirectory(icons)
add_subdirectory(man)
add_subdirectory(misc)
add_subdirectory(pkgconfig)

# Install gresource XML
install(FILES pchat.gresource.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/pchat)
