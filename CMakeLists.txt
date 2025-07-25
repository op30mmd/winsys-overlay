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

find_package(Qt6 REQUIRED COMPONENTS Widgets)

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

    # Add a post-build step to run windeployqt with optimizations.
    # This command now correctly calls the executable and passes all the
    # necessary arguments to reduce the final deployment size.
    add_custom_command(TARGET winsys-overlay POST_BUILD
        # Use the correct variable provided by Qt for the windeployqt executable
        COMMAND ${Qt6_windeployqt_EXECUTABLE}
        # Add arguments to reduce deployment size
        --release
        --no-translations
        --no-opengl-sw
        --no-system-d3d-compiler
        --no-vc-redist
        # Specify output directory and the executable to deploy
        --dir "$<TARGET_FILE_DIR:winsys-overlay>"
        "$<TARGET_FILE:winsys-overlay>"
        COMMENT "Running optimized windeployqt on $<TARGET_FILE_NAME:winsys-overlay>..."
        VERBATIM
    )
endif()
