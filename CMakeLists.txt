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

# Install the main executable and associate it with the "Application" component
install(TARGETS winsys-overlay RUNTIME DESTINATION bin COMPONENT Application)

# Use windeployqt to deploy Qt dependencies into the installation directory
install(CODE "
    message(STATUS \"Running windeployqt...\")
    execute_process(
        COMMAND ${Qt6_windeployqt_EXECUTABLE} --release --no-translations --dir \"${CMAKE_INSTALL_PREFIX}/bin\" \"$<TARGET_FILE:winsys-overlay>\"
    )
" COMPONENT Application)

# Install the C# helper application and its dependency
install(FILES
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/TempReader.exe"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/LibreHardwareMonitorLib.dll"
    DESTINATION bin
    COMPONENT Application
)

# --- CPack Configuration ---

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "1.0.0")
# Fix Vendor/Developer name
set(CPACK_PACKAGE_VENDOR "op30mmd")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A simple system information overlay for Windows.")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
set(CPACK_GENERATOR "NSIS")

# Include the license file in the installer
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

# NSIS specific settings
set(CPACK_NSIS_PACKAGE_NAME "${PROJECT_NAME} ${CPACK_PACKAGE_VERSION} Installer")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
set(CPACK_NSIS_COMPONENT_SECTION_NAME_ENABLE ON)
set(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_NAME} version ${CPACK_PACKAGE_VERSION}")
set(CPACK_NSIS_INSTALLED_ICON_NAME "winsys-overlay.exe")
set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$DESKTOP\\\\${CPACK_PACKAGE_NAME}.lnk' '$INSTDIR\\\\bin\\\\winsys-overlay.exe'")
set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$DESKTOP\\\\${CPACK_PACKAGE_NAME}.lnk'")


# Define components for CPack
set(CPACK_COMPONENTS_ALL Application)
# Corrected install command for the license file
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" DESTINATION "." RENAME "LICENSE.txt" COMPONENT Application)


include(CPack)
