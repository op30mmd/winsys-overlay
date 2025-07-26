
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

# --- C# Helper Application ---

# Add a custom target to build the C# TempReader project
add_custom_target(TempReader ALL
    COMMAND dotnet build "${CMAKE_CURRENT_SOURCE_DIR}/TempReader.csproj" -c Release -o "${CMAKE_BINARY_DIR}/bin/Release"
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

# Add a dependency to ensure TempReader is built before the main application
add_dependencies(winsys-overlay TempReader)

# Link against standard libraries
target_link_libraries(winsys-overlay PRIVATE 
    Qt6::Widgets 
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
    install(TARGETS winsys-overlay
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
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
