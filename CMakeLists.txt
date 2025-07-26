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

install(TARGETS winsys-overlay RUNTIME DESTINATION bin)

install(CODE "
    message(STATUS \"Running windeployqt...\")
    execute_process(
        COMMAND ${Qt6_windeployqt_EXECUTABLE} --release --no-translations --dir \"${CMAKE_INSTALL_PREFIX}/bin\" \"$<TARGET_FILE:winsys-overlay>\"
    )
")

install(FILES
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/TempReader.exe"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release/LibreHardwareMonitorLib.dll"
    DESTINATION bin
)

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_VENDOR "Gemini")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A simple system information overlay for Windows.")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
set(CPACK_GENERATOR "NSIS")

include(CPack)



