cmake_minimum_required(VERSION 3.16)

project(WinSysOverlay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

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

    # Add a post-build step to run windeployqt on the executable
    add_custom_command(TARGET winsys-overlay POST_BUILD
        COMMAND ${WINDEPLOYQT_EXECUTABLE}
        ARGS --dir "$<TARGET_FILE_DIR:winsys-overlay>" "$<TARGET_FILE:winsys-overlay>"
        COMMENT "Running windeployqt on $<TARGET_FILE_NAME:winsys-overlay>..."
        VERBATIM
    )
endif()