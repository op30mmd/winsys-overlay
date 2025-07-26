cmake_minimum_required(VERSION 3.26)
project(WinSysOverlay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Explicitly tell CMake where to find your Qt installation.
set(CMAKE_PREFIX_PATH "C:/Qt/6.9.1/msvc2022_64" CACHE PATH "Path to Qt installation")
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# --- C++/CLI Wrapper Library ---

add_library(HardwareMonitorWrapper SHARED HardwareMonitor.cpp HardwareMonitor.h)

# Set the modern CMake properties for C++/CLI compilation
set_property(TARGET HardwareMonitorWrapper PROPERTY COMMON_LANGUAGE_RUNTIME "") # Use .NET Framework
set_property(TARGET HardwareMonitorWrapper PROPERTY DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2")
set_property(TARGET HardwareMonitorWrapper PROPERTY VS_PROJECT_IMPORT "${CMAKE_CURRENT_SOURCE_DIR}/winrt.props")

# Find and reference the external LibreHardwareMonitorLib.dll
find_file(LHM_DLL_PATH LibreHardwareMonitorLib.dll HINTS "${CMAKE_CURRENT_SOURCE_DIR}/libs/lhm")
if(LHM_DLL_PATH)
    # Use /FU to reference the .NET assembly at compile time
    target_compile_options(HardwareMonitorWrapper PRIVATE "/FU${LHM_DLL_PATH}")
    # Copy the DLL to the output directory for runtime
    add_custom_command(TARGET HardwareMonitorWrapper POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LHM_DLL_PATH}"
        "$<TARGET_FILE_DIR:HardwareMonitorWrapper>"
    )
else()
    message(FATAL_ERROR "LibreHardwareMonitorLib.dll not found!")
endif()

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

# The Correct Solution: This automatically handles linker paths for all build configurations.
# The incorrect target_link_directories has been removed.
target_link_libraries(winsys-overlay PRIVATE 
    Qt6::Widgets 
    HardwareMonitorWrapper
    pdh psapi iphlpapi ws2_32 wbemuuid
)

# --- Deployment ---

# Find windeployqt
if(NOT Qt6_windeployqt_EXECUTABLE)
    find_program(Qt6_windeployqt_EXECUTABLE
        NAMES windeployqt.exe windeployqt
        PATHS ${CMAKE_PREFIX_PATH}/bin
        NO_DEFAULT_PATH
    )
endif()
if(NOT Qt6_windeployqt_EXECUTABLE)
    message(FATAL_ERROR "Failed to find windeployqt.exe.")
endif()

get_filename_component(QT_BIN_DIR "${Qt6_windeployqt_EXECUTABLE}" DIRECTORY)

if(WIN32)
    include(GNUInstallDirs)
    install(TARGETS winsys-overlay HardwareMonitorWrapper
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    add_custom_command(TARGET winsys-overlay POST_BUILD
        COMMAND "${Qt6_windeployqt_EXECUTABLE}"
            --release --no-translations --no-opengl-sw --no-system-d3d-compiler
            --dir "$<TARGET_FILE_DIR:winsys-overlay>"
            "$<TARGET_FILE:winsys-overlay>"
        WORKING_DIRECTORY "${QT_BIN_DIR}"
        COMMENT "Running windeployqt on $<TARGET_FILE_NAME:winsys-overlay>..."
        VERBATIM
    )
endif()