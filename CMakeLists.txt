cmake_minimum_required(VERSION 3.16)
project(WinSysOverlay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Set the correct exception handling model for C++/CLI
add_compile_options(/EHa)

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

# Explicitly set the compiler flags for the C++/CLI file, including the assembly search path
set_source_files_properties(HardwareMonitor.cpp PROPERTIES
    COMPILE_FLAGS "/clr /AI\"C:/Program Files (x86)/Reference Assemblies/Microsoft/Framework/.NETFramework/v4.7.2\""
)

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