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

# --- Clean Deployment ---

# Create a clean install directory structure
# Install only the specific files we need, nothing else

# Main application executable
install(
    FILES "${CMAKE_BINARY_DIR}/bin/Release/winsys-overlay.exe"
    DESTINATION "bin"
    COMPONENT Application
)

# C# helper executable and its dependency
install(
    FILES 
        "${CMAKE_BINARY_DIR}/bin/Release/TempReader.exe"
        "${CMAKE_BINARY_DIR}/bin/Release/LibreHardwareMonitorLib.dll"
    DESTINATION "bin"
    COMPONENT Application
)

# License file
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" 
    DESTINATION "." 
    RENAME "LICENSE.txt" 
    COMPONENT Application
)

# Custom install script to run windeployqt and clean up during CPack install
install(CODE "
    set(QT_DEPLOY_DIR \"\${CMAKE_INSTALL_PREFIX}/bin\")
    message(STATUS \"Installing to: \${CMAKE_INSTALL_PREFIX}\")
    message(STATUS \"Qt deploy directory: \${QT_DEPLOY_DIR}\")
    
    # Find windeployqt
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS \"${CMAKE_PREFIX_PATH}/bin\")
    
    if(NOT WINDEPLOYQT_EXECUTABLE)
        message(FATAL_ERROR \"windeployqt not found at ${CMAKE_PREFIX_PATH}/bin\")
    endif()
    
    message(STATUS \"Found windeployqt at: \${WINDEPLOYQT_EXECUTABLE}\")
    
    # Run windeployqt with minimal options
    execute_process(
        COMMAND \"\${WINDEPLOYQT_EXECUTABLE}\" 
            --release 
            --no-translations 
            --no-system-d3d-compiler 
            --no-opengl-sw 
            --no-compiler-runtime
            --no-quick-import
            --force
            \"\${QT_DEPLOY_DIR}/winsys-overlay.exe\"
        WORKING_DIRECTORY \"\${QT_DEPLOY_DIR}\"
        RESULT_VARIABLE DEPLOY_RESULT
        OUTPUT_VARIABLE DEPLOY_OUTPUT
        ERROR_VARIABLE DEPLOY_ERROR
    )
    
    message(STATUS \"windeployqt output: \${DEPLOY_OUTPUT}\")
    
    if(NOT DEPLOY_RESULT EQUAL 0)
        message(WARNING \"windeployqt failed with code \${DEPLOY_RESULT}: \${DEPLOY_ERROR}\")
        message(WARNING \"Continuing without Qt deployment - app may not work without Qt runtime\")
    else()
        message(STATUS \"windeployqt completed successfully\")
        
        # Remove any unnecessary Qt DLLs that windeployqt might have copied
        set(UNNECESSARY_PATTERNS
            \"Qt6Quick*.dll\"
            \"Qt6Qml*.dll\" 
            \"Qt6Network*.dll\"
            \"Qt6Sql*.dll\"
            \"Qt6Test*.dll\"
            \"Qt6WebEngine*.dll\"
            \"Qt6Multimedia*.dll\"
            \"opengl32sw.dll\"
            \"d3dcompiler_*.dll\"
        )
        
        foreach(PATTERN \${UNNECESSARY_PATTERNS})
            file(GLOB FILES_TO_REMOVE \"\${QT_DEPLOY_DIR}/\${PATTERN}\")
            foreach(FILE_TO_REMOVE \${FILES_TO_REMOVE})
                file(REMOVE \"\${FILE_TO_REMOVE}\")
                get_filename_component(FILENAME \"\${FILE_TO_REMOVE}\" NAME)
                message(STATUS \"Removed unnecessary file: \${FILENAME}\")
            endforeach()
        endforeach()
        
        # Clean up platform plugins (keep only qwindows.dll)
        set(PLATFORMS_DIR \"\${QT_DEPLOY_DIR}/platforms\")
        if(EXISTS \"\${PLATFORMS_DIR}\")
            file(GLOB PLATFORM_PLUGINS \"\${PLATFORMS_DIR}/*.dll\")
            foreach(PLUGIN \${PLATFORM_PLUGINS})
                get_filename_component(PLUGIN_NAME \"\${PLUGIN}\" NAME)
                if(NOT PLUGIN_NAME STREQUAL \"qwindows.dll\")
                    file(REMOVE \"\${PLUGIN}\")
                    message(STATUS \"Removed platform plugin: \${PLUGIN_NAME}\")
                endif()
            endforeach()
        endif()
        
        # Clean up imageformat plugins (keep only essential ones)
        set(IMAGEFORMATS_DIR \"\${QT_DEPLOY_DIR}/imageformats\")
        if(EXISTS \"\${IMAGEFORMATS_DIR}\")
            file(GLOB IMAGE_PLUGINS \"\${IMAGEFORMATS_DIR}/*.dll\")
            set(KEEP_PLUGINS \"qico.dll\" \"qjpeg.dll\" \"qpng.dll\" \"qgif.dll\")
            foreach(PLUGIN \${IMAGE_PLUGINS})
                get_filename_component(PLUGIN_NAME \"\${PLUGIN}\" NAME)
                list(FIND KEEP_PLUGINS \"\${PLUGIN_NAME}\" FOUND_INDEX)
                if(FOUND_INDEX EQUAL -1)
                    file(REMOVE \"\${PLUGIN}\")
                    message(STATUS \"Removed image plugin: \${PLUGIN_NAME}\")
                endif()
            endforeach()
        endif()
    endif()
    
    # List final contents for verification
    message(STATUS \"Final installation contents:\")
    file(GLOB_RECURSE ALL_INSTALLED_FILES \"\${CMAKE_INSTALL_PREFIX}/*\")
    foreach(INSTALLED_FILE \${ALL_INSTALLED_FILES})
        file(RELATIVE_PATH REL_PATH \"\${CMAKE_INSTALL_PREFIX}\" \"\${INSTALLED_FILE}\")
        message(STATUS \"  \${REL_PATH}\")
    endforeach()
    
" COMPONENT Application)
set(CPACK_SOURCE_IGNORE_FILES
    "/build/"
    "/.git/"
    "/.github/"
    "/CMakeFiles/"
    "CMakeCache.txt"
    "/*.vcxproj"
    "/*.sln" 
    "/Debug/"
    "/Release/"
    "/.vs/"
    "/x64/"
    "/_CPack_Packages/"
    "/CPackConfig.cmake"
    "/CPackSourceConfig.cmake"
    "/*.dir/"
)

# Exclude build artifacts from installation
set(CPACK_IGNORE_FILES
    "/build/"
    "/_CPack_Packages/"
    "/CMakeFiles/"
    "CMakeCache.txt"
    "/x64/"
    "/*.dir/"
    "/ALL_BUILD.vcxproj"
    "/ZERO_CHECK.vcxproj"
    "/PACKAGE.vcxproj"
    "/INSTALL.vcxproj"
    "/*.vcxproj.filters"
    "/*.sln"
)

# --- CPack Configuration ---

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "op30mmd")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A simple system information overlay for Windows.")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
set(CPACK_GENERATOR "NSIS")

set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-win64")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

# NSIS specific settings
set(CPACK_NSIS_PACKAGE_NAME "${PROJECT_NAME} ${CPACK_PACKAGE_VERSION}")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_NAME}")
set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\winsys-overlay.exe")

# Create desktop shortcut
set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$DESKTOP\\\\${CPACK_PACKAGE_NAME}.lnk' '$INSTDIR\\\\bin\\\\winsys-overlay.exe'")
set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$DESKTOP\\\\${CPACK_PACKAGE_NAME}.lnk'")

# Enable solid LZMA compression for smallest installer
set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

# Only include the Application component
set(CPACK_COMPONENTS_ALL Application)

# Don't include any source or build files
set(CPACK_SOURCE_IGNORE_FILES
    "/build/"
    "/.git/"
    "/.github/"
    "/CMakeFiles/"
    "CMakeCache.txt"
    "/*.vcxproj"
    "/*.sln"
    "/Debug/"
    "/Release/"
    "/.vs/"
)

include(CPack)