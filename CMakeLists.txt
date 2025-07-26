cmake_minimum_required(VERSION 3.16)
project(WinSysOverlay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Enable C++/CLI support and set the correct exception handling model
set_property(GLOBAL PROPERTY USE_MSVC_RUNTIME_LIBRARY_DLL ON)
add_compile_options(/EHa)

# Explicitly tell CMake where to find your Qt installation.
set(CMAKE_PREFIX_PATH "C:/Qt/6.9.1/msvc2022_64" CACHE PATH "Path to Qt installation")
message(STATUS "Using CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Find windeployqt manually if not found by Qt6 package
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

message(STATUS "Found windeployqt: ${Qt6_windeployqt_EXECUTABLE}")
get_filename_component(QT_BIN_DIR "${Qt6_windeployqt_EXECUTABLE}" DIRECTORY)

add_executable(winsys-overlay WIN32
    main.cpp
    overlaywidget.h
    overlaywidget.cpp
    sysinfomonitor.h
    sysinfomonitor.cpp
    settingsdialog.h
    settingsdialog.cpp
    HardwareMonitor.h
    HardwareMonitor.cpp
)

# Set the .NET Framework version for the C++/CLI parts
set_property(TARGET winsys-overlay PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2")

# Tell CMake that this specific file is managed code
set_source_files_properties(HardwareMonitor.cpp PROPERTIES VS_COMPILE_AS_MANAGED "Managed")

target_link_libraries(winsys-overlay PRIVATE 
    Qt6::Widgets 
    pdh psapi iphlpapi ws2_32 wbemuuid
)

if(WIN32)
    include(GNUInstallDirs)
    install(TARGETS winsys-overlay RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

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