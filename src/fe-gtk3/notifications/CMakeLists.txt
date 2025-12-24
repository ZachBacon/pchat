# Notification backend for Windows
# Either WinRT (Windows 8+) or simple Shell_NotifyIcon (all Windows versions)

if(WIN32)
    if(ENABLE_WINRT_NOTIFICATIONS)
        # Enable C++ language for WinRT version
        enable_language(CXX)
        
        if(MSVC)
            # MSVC: Build the C++/CX version
            message(STATUS "Building WinRT notification plugin with C++/CX (MSVC)")
            
            add_library(pcnotifications-winrt SHARED notification-winrt.cpp)
            
            # Explicitly set language
            set_target_properties(pcnotifications-winrt PROPERTIES LINKER_LANGUAGE CXX)
            
            # Set C++/CX compilation flags
            target_compile_options(pcnotifications-winrt PRIVATE
                /ZW           # Enable C++/CX (Windows Runtime extensions)
                /EHsc         # Exception handling
                /W3           # Warning level
            )
            
            # Link against Windows Runtime libraries
            target_link_libraries(pcnotifications-winrt PRIVATE
                WindowsApp    # Windows Runtime library
            )
            
        elseif(MINGW OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            # MinGW/Clang: Build the COM-based version
            message(STATUS "Building WinRT notification plugin with COM APIs (MinGW/Clang)")
            
            add_library(pcnotifications-winrt SHARED notification-winrt-mingw.cpp)
            
            # Explicitly set language
            set_target_properties(pcnotifications-winrt PROPERTIES LINKER_LANGUAGE CXX)
            
            # C++ standard and options
            target_compile_features(pcnotifications-winrt PRIVATE cxx_std_11)
            target_compile_options(pcnotifications-winrt PRIVATE
                -municode
                -D_UNICODE
                -DUNICODE
            )
            
            # Link against Windows Runtime libraries
            target_link_libraries(pcnotifications-winrt PRIVATE
                runtimeobject    # Windows Runtime
                ole32
                oleaut32
            )
        endif()
        
        if(TARGET pcnotifications-winrt)
            # Set output name to match what notification-windows.c expects
            set_target_properties(pcnotifications-winrt PROPERTIES
                OUTPUT_NAME "pcnotifications-winrt"
                PREFIX ""
                SUFFIX ".dll"
            )
            
            # Install the DLL to the plugins directory
            install(TARGETS pcnotifications-winrt
                RUNTIME DESTINATION ${PCHAT_LIBDIR}
                COMPONENT Runtime
            )
            
            # Set a variable that parent CMakeLists can check
            set(HAVE_WINRT_NOTIFICATIONS TRUE PARENT_SCOPE)
            message(STATUS "WinRT notification plugin will be built")
        else()
            message(WARNING "WinRT notification plugin could not be built - falling back to simple notifications")
            set(HAVE_WINRT_NOTIFICATIONS FALSE PARENT_SCOPE)
        endif()
    else()
        # Use simple Shell_NotifyIcon notifications
        message(STATUS "Using simple Shell_NotifyIcon notifications (ENABLE_WINRT_NOTIFICATIONS=OFF)")
        set(HAVE_WINRT_NOTIFICATIONS FALSE PARENT_SCOPE)
    endif()
else()
    message(STATUS "Skipping Windows notification backends (Windows only)")
    set(HAVE_WINRT_NOTIFICATIONS FALSE PARENT_SCOPE)
endif()
