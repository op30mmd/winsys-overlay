cmake_minimum_required(VERSION 3.16)
project(WinSysOverlay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Enable Link-Time Optimization (LTO) for smaller executables
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported)
if(ipo_supported)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  message(STATUS "IPO/LTO enabled.")
endif()

# Explicitly tell CMake where to find your Qt installation.
# This should be the path to the folder containing bin, lib, include, etc.
set(CMAKE_PREFIX_PATH "C:/Qt/6.9.1/msvc2022_64" CACHE PATH "Path to Qt installation")
message(STATUS "Using CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Find windeployqt manually if not found by Qt6 package
if(NOT Qt6_windeployqt_EXECUTABLE)
    # Try to find windeployqt in the Qt bin directory
    find_program(Qt6_windeployqt_EXECUTABLE
        NAMES windeployqt.exe windeployqt
        PATHS ${CMAKE_PREFIX_PATH}/bin
        NO_DEFAULT_PATH
    )
endif()

# Check if we found windeployqt
if(NOT Qt6_windeployqt_EXECUTABLE)
    message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")
    message(STATUS "Looking for windeployqt in: ${CMAKE_PREFIX_PATH}/bin")
    message(FATAL_ERROR "Failed to find windeployqt.exe. Ensure your Qt installation is correct and CMAKE_PREFIX_PATH is set properly.")
endif()

message(STATUS "Found windeployqt: ${Qt6_windeployqt_EXECUTABLE}")

# Get the directory containing the Qt binaries. This is crucial for the post-build step.
get_filename_component(QT_BIN_DIR "${Qt6_windeployqt_EXECUTABLE}" DIRECTORY)

add_executable(winsys-overlay WIN32
    main.cpp
    overlaywidget.h
    overlaywidget.cpp
    sysinfomonitor.h
    sysinfomonitor.cpp
    settingsdialog.h
    settingsdialog.cpp
)

# Link against the Performance Data Helper library on Windows
target_link_libraries(winsys-overlay PRIVATE Qt6::Widgets pdh)

if(WIN32)
    include(GNUInstallDirs)
    install(TARGETS winsys-overlay
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    # --- Fixed Deployment Method ---
    # Use add_custom_command directly with proper escaping
    add_custom_command(TARGET winsys-overlay POST_BUILD
        COMMAND "${Qt6_windeployqt_EXECUTABLE}"
            --release
            --no-translations
            --no-opengl-sw
            --no-system-d3d-compiler
            --dir "$<TARGET_FILE_DIR:winsys-overlay>"
            "$<TARGET_FILE:winsys-overlay>"
        WORKING_DIRECTORY "${QT_BIN_DIR}"
        COMMENT "Running windeployqt on $<TARGET_FILE_NAME:winsys-overlay>..."
        VERBATIM
    )
endif()