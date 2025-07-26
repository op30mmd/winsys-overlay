
cmake_minimum_required(VERSION 3.16)
project(WinSysOverlay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Set the correct properties for C++/CLI compilation
set(CMAKE_COMMON_LANGUAGE_RUNTIME_SUPPORT ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHa")

# Explicitly tell CMake where to find your Qt installation.
set(CMAKE_PREFIX_PATH "C:/Qt/6.9.1/msvc2022_64" CACHE PATH "Path to Qt installation")
find_package(Qt6 REQUIRED COMPONENTS Widgets)

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

# Set the .NET Framework version for the target
set_target_properties(winsys-overlay PROPERTIES
    VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.7.2"
)

# Mark only the specific file as managed code
set_source_files_properties(HardwareMonitor.cpp PROPERTIES
    VS_COMPILE_AS_MANAGED "Managed"
)

# Unset the global property so it doesn't affect other potential targets
set(CMAKE_COMMON_LANGUAGE_RUNTIME_SUPPORT OFF)

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
