# Plugins
if(ENABLE_PYTHON)
    add_subdirectory(python)
endif()

# AudioPlayer plugin (optional, requires FFmpeg and FAudio)
if(ENABLE_AUDIOPLAYER)
    add_subdirectory(audioplayer)
endif()

# Cross-platform plugins
if(ENABLE_CHECKSUM)
    add_subdirectory(checksum)
endif()

if(ENABLE_FISHLIM)
    add_subdirectory(fishlim)
endif()

if(ENABLE_LUA)
    add_subdirectory(lua)
endif()

if(ENABLE_SYSINFO)
    add_subdirectory(sysinfo)
endif()

# Windows-specific plugins
if(WIN32)
    if(ENABLE_EXEC)
        add_subdirectory(exec)
    endif()
    
    if(ENABLE_WINAMP)
        add_subdirectory(winamp)
    endif()
    
    if(ENABLE_UPD)
        add_subdirectory(upd)
    endif()
endif()
