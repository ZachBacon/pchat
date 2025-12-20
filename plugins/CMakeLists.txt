# Plugins
if(ENABLE_PYTHON)
    add_subdirectory(python)
endif()

if(ENABLE_PERL)
    add_subdirectory(perl)
endif()

# AudioPlayer plugin (optional, requires FFmpeg and FAudio)
if(ENABLE_AUDIOPLAYER)
    add_subdirectory(audioplayer)
endif()
