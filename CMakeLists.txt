cmake_minimum_required(VERSION 3.26)
project(WinSysOverlay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Define a unified output directory for all build artifacts
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Explicitly tell CMake where to find your Qt installation.
set(CMAKE_PREFIX_PATH "C:/Qt/6.9.1/msvc2022_64" CACHE PATH "Path to Qt installation")
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# --- C# Helper Application ---

# Find the required LHM library
find_file(LHM_DLL_PATH LibreHardwareMonitorLib.dll HINTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/lhm")
if(NOT LHM_DLL_PATH)
    message(FATAL_ERROR "LibreHardwareMonitorLib.dll not found in libs/lhm!")
endif()

# Add a custom target to build the C# TempReader project and copy its dependency
add_custom_target(TempReader ALL
    # Build the C# project and output the .exe to our unified bin directory
    COMMAND dotnet build "${CMAKE_CURRENT_SOURCE_DIR}/TempReader.csproj" -c Release -o "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release"
    # Copy the required LHM DLL to the same directory so the .exe can find it
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LHM_DLL_PATH}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building C# TempReader helper..."
)

# --- Main Application ---

add_executable(winsys-overlay WIN32
    main.cpp
    overlaywidget.h
    overlaywidget.cpp
    sysinfomonitor.h
    sysinfomonitor.cpp
    settingsdialog.h
    settingsdialog.cpp
)

# Ensure TempReader is built before the main application
add_dependencies(winsys-overlay TempReader)

target_link_libraries(winsys-overlay PRIVATE
    Qt6::Widgets
    pdh psapi iphlpapi ws2_32 wbemuuid
)

# --- Deployment & Installation ---

# Install only the essential files
set(DEPLOY_SOURCE_DIR "${CMAKE_BINARY_DIR}/bin/Release")

# Install main executable
install(
    FILES "${DEPLOY_SOURCE_DIR}/winsys-overlay.exe"
    DESTINATION "bin"
    COMPONENT Application
)

# Install C# helper and its dependency
install(
    FILES 
        "${DEPLOY_SOURCE_DIR}/TempReader.exe"
        "${DEPLOY_SOURCE_DIR}/LibreHardwareMonitorLib.dll"
    DESTINATION "bin"
    COMPONENT Application
)

# Install only essential Qt DLLs (will be populated by windeployqt)
# Note: This will be handled by a custom install script
install(CODE "
    # Find windeployqt
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS \${CMAKE_PREFIX_PATH}/bin)
    
    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS \"Running windeployqt on \${CMAKE_INSTALL_PREFIX}/bin/winsys-overlay.exe\")
        execute_process(
            COMMAND \"\${WINDEPLOYQT_EXECUTABLE}\" 
                --release 
                --no-translations 
                --no-system-d3d-compiler 
                --no-opengl-sw 
                --no-compiler-runtime
                --force
                \"\${CMAKE_INSTALL_PREFIX}/bin/winsys-overlay.exe\"
            WORKING_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/bin\"
            RESULT_VARIABLE WINDEPLOYQT_RESULT
        )
        
        if(NOT WINDEPLOYQT_RESULT EQUAL 0)
            message(WARNING \"windeployqt failed with result: \${WINDEPLOYQT_RESULT}\")
        endif()
        
        # Remove unnecessary Qt files that windeployqt might have copied
        file(GLOB UNNECESSARY_FILES
            \"\${CMAKE_INSTALL_PREFIX}/bin/Qt6Quick*\"
            \"\${CMAKE_INSTALL_PREFIX}/bin/Qt6Qml*\"
            \"\${CMAKE_INSTALL_PREFIX}/bin/Qt6Network*\"
            \"\${CMAKE_INSTALL_PREFIX}/bin/Qt6Sql*\"
            \"\${CMAKE_INSTALL_PREFIX}/bin/Qt6Test*\"
            \"\${CMAKE_INSTALL_PREFIX}/bin/Qt6WebEngine*\"
            \"\${CMAKE_INSTALL_PREFIX}/bin/Qt6Multimedia*\"
            \"\${CMAKE_INSTALL_PREFIX}/bin/opengl32sw.dll\"
            \"\${CMAKE_INSTALL_PREFIX}/bin/d3dcompiler_*.dll\"
        )
        
        foreach(UNNECESSARY_FILE \${UNNECESSARY_FILES})
            if(EXISTS \"\${UNNECESSARY_FILE}\")
                file(REMOVE \"\${UNNECESSARY_FILE}\")
                message(STATUS \"Removed unnecessary file: \${UNNECESSARY_FILE}\")
            endif()
        endforeach()
        
        # Remove unnecessary platform plugins (keep only qwindows.dll)
        file(GLOB PLATFORM_PLUGINS \"\${CMAKE_INSTALL_PREFIX}/bin/platforms/*.dll\")
        foreach(PLUGIN \${PLATFORM_PLUGINS})
            get_filename_component(PLUGIN_NAME \"\${PLUGIN}\" NAME)
            if(NOT PLUGIN_NAME STREQUAL \"qwindows.dll\")
                file(REMOVE \"\${PLUGIN}\")
                message(STATUS \"Removed unnecessary platform plugin: \${PLUGIN}\")
            endif()
        endforeach()
        
        # Remove imageformats we don't need (keep basic ones)
        file(GLOB IMAGE_PLUGINS \"\${CMAKE_INSTALL_PREFIX}/bin/imageformats/*.dll\")
        set(KEEP_IMAGE_PLUGINS \"qico.dll\" \"qjpeg.dll\" \"qpng.dll\" \"qgif.dll\")
        foreach(PLUGIN \${IMAGE_PLUGINS})
            get_filename_component(PLUGIN_NAME \"\${PLUGIN}\" NAME)
            list(FIND KEEP_IMAGE_PLUGINS \"\${PLUGIN_NAME}\" KEEP_INDEX)
            if(KEEP_INDEX EQUAL -1)
                file(REMOVE \"\${PLUGIN}\")
                message(STATUS \"Removed unnecessary image plugin: \${PLUGIN}\")
            endif()
        endforeach()
        
    else()
        message(WARNING \"windeployqt not found - Qt libraries may not be deployed correctly\")
    endif()
" COMPONENT Application)

# --- CPack Configuration ---

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "op30mmd")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A simple system information overlay for Windows.")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
set(CPACK_GENERATOR "NSIS")

set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-win64")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

# NSIS specific settings for smaller package
set(CPACK_NSIS_PACKAGE_NAME "${PROJECT_NAME} ${CPACK_PACKAGE_VERSION} Installer")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
set(CPACK_NSIS_COMPONENT_SECTION_NAME_ENABLE ON)
set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_NAME} version ${CPACK_PACKAGE_VERSION}")
set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\winsys-overlay.exe")
set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$DESKTOP\\\\${CPACK_PACKAGE_NAME}.lnk' '$INSTDIR\\\\bin\\\\winsys-overlay.exe'")
set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$DESKTOP\\\\${CPACK_PACKAGE_NAME}.lnk'")

# Enable compression for smaller installer
set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

# Exclude common unnecessary files
set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
    ; Remove any debug files that might have been included
    Delete \\\"$INSTDIR\\\\bin\\\\*.pdb\\\"
    Delete \\\"$INSTDIR\\\\bin\\\\*d.dll\\\"
    RMDir /r \\\"$INSTDIR\\\\bin\\\\debug\\\"
")

# Define components for CPack
set(CPACK_COMPONENTS_ALL Application)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" DESTINATION "." RENAME "LICENSE.txt" COMPONENT Application)

include(CPack)