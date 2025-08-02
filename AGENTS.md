# AGENTS.md

## Introduction

Welcome, AI agent. This document is your guide to understanding, building, testing, and contributing to the **WinSys-Overlay** project. Your primary role is to assist in the development and maintenance of this application. Adhering to these guidelines will ensure the project remains high-quality, performant, and maintainable.

-----

## Agent Limitations

**IMPORTANT:** As an AI agent, you operate in a command-line environment and **cannot interact with any Graphical User Interface (GUI)**. This application is a GUI tool. Therefore, your testing is limited to the following:

  * Ensuring the project builds successfully.
  * Analyzing code for potential bugs, logical errors, and performance issues.
  * Implementing new code logic.

You cannot perform manual UI tests. The "Manual Testing Checklist" is provided for human developers. Your role is to ensure the code is sound and the application compiles without errors.

-----

## Project Overview

**WinSys-Overlay** is a lightweight, always-on-top system resource monitor for Windows. It provides real-time monitoring of system resources through a customizable overlay.

### Key Technologies:

  * **Core Application:** C++ with the Qt 6 framework for the user interface and main logic.
  * **Temperature Monitoring:** A separate C\# helper application (`TempReader.exe`) that uses `LibreHardwareMonitorLib.dll` to fetch CPU and GPU temperatures.
  * **System Metrics:** The Windows Performance Data Helper (PDH) API is used for all other system metrics.
  * **Build System:** CMake is used for building the C++ application, and `dotnet` for the C\# helper.
  * **CI/CD:** GitHub Actions are set up for automated builds and releases.

-----

## Core Responsibilities

As an AI agent, your tasks will include:

  * **Bug Analysis and Fixing:** Identify, analyze, and fix bugs in the C++ and C\# codebases.
  * **Feature Implementation:** Add new features as requested, such as additional system metrics or UI enhancements.
  * **Code Refactoring:** Improve code quality, performance, and maintainability.
  * **Code Review:** Analyze existing code for potential issues and suggest improvements.

-----

## Building the Project

### Cross-compiling for Windows from a Linux Environment

Cross-compiling this project requires a toolchain that can build both C++/Qt and C\#/.NET applications for Windows from a Linux host.

1.  **Set up the C++ toolchain:**
      * Install **MinGW-w64**. **Always use `sudo`** when installing packages (e.g., `sudo apt-get install mingw-w64`).
      * Install a cross-compiled version of the **Qt 6 framework** for MinGW-w64.
2.  **Set up the C\# toolchain:**
      * Install the **.NET SDK**.
3.  **Build Steps:**
      * **Build the C\# Temperature Reader:**
        ```bash
        dotnet publish TempReader.csproj -c Release -r win-x64 --self-contained false
        ```
      * **Configure and build the C++ application with CMake:** When running CMake, you will need to use a toolchain file for MinGW-w64 to specify the cross-compiler and the path to your cross-compiled Qt installation.

### Building on Windows (Native)

The `README.md` provides a good overview of the native build process on Windows. Refer to it for prerequisites.

The build process is also automated via the GitHub Actions workflow in `.github/workflows/release.yml`.

-----

## Testing

**Always check for bugs.** Proactively look for potential issues, even if they are not immediately obvious.

### Agent-Based Testing

Given your limitations, your testing process should focus on:

  * **Successful Compilation:** Ensure that both the C++ and C\# projects build successfully without any errors or warnings.
  * **Static Analysis:** Analyze the code for potential null pointer exceptions, race conditions, memory leaks, and other common programming errors.
  * **Dependency Checks:** Verify that all required DLLs and dependencies are correctly located and referenced in the build scripts.

### Manual Testing Checklist (for Human Developers)

  * **UI Functionality:**
      * Can the overlay be dragged and repositioned?
      * Does the right-click context menu work as expected?
      * Does the settings dialog open and close correctly?
      * Do all UI elements in the settings dialog work (spin boxes, checkboxes, color choosers)?
  * **Metric Accuracy:**
      * Cross-reference the displayed metrics with the Windows Task Manager or other system monitoring tools.
      * Ensure that temperature readings are within a reasonable range.
  * **Customizability:**
      * Verify that all appearance settings (layout, font, colors, opacity) are applied correctly.
      * Test that showing/hiding individual metrics works as expected.
  * **Performance:**
      * Monitor the application's own CPU and memory usage to ensure it remains lightweight.
      * Test different update intervals and observe their impact on system performance.

### Troubleshooting the Test Environment

You may encounter the following CMake error during configuration if the path to the Qt 6 SDK is not correctly specified:

```
CMake Error at CMakeLists.txt:35 (find_package):
  By not providing "FindQt6.cmake" in CMAKE_MODULE_PATH this project has
  asked CMake to find a package configuration file provided by "Qt6", but
  CMake did not find one.
```

This occurs because the `CMakeLists.txt` file may have a hardcoded path to a Qt installation that does not exist on your test machine.

**To resolve this (for testing only):**

1.  **Temporarily Modify `CMakeLists.txt`:** Use your internal editing capabilities to modify the `CMakeLists.txt` file in memory. Comment out or remove the line that sets a hardcoded `CMAKE_PREFIX_PATH`. **Do not use a terminal text editor like `vim` or `nano`.**
      * **Change this:**
        ```cmake
        set(CMAKE_PREFIX_PATH "C:/Qt/6.9.1/msvc2022_64" CACHE PATH "Path to Qt installation")
        find_package(Qt6 REQUIRED COMPONENTS Widgets)
        ```
      * **To this:**
        ```cmake
        # The CMAKE_PREFIX_PATH will be passed via the command line instead.
        find_package(Qt6 REQUIRED COMPONENTS Widgets)
        ```
2.  **Run CMake with the Correct Path:** From your `build` directory, execute the `cmake` command and provide the correct path to your Qt installation using the `-DCMAKE_PREFIX_PATH` argument, as instructed in the project's `README.md`.
    ```bash
    cmake .. -DCMAKE_PREFIX_PATH="C:\path\to\your\Qt"
    ```
3.  **Discard Changes:** After the build is complete, **discard your in-memory changes to `CMakeLists.txt`**. Do not include the modification in any patches or commits to production.

-----

## Things to Avoid

  * **Do not introduce new dependencies** without careful consideration. The project is designed to be lightweight.
  * **Do not break the hybrid C++/C\# architecture.** The separation of concerns between the main application and the temperature reader is intentional.
  * **Do not hardcode paths or values.** Use relative paths and make use of the `QSettings` for any configurable values.
  * **Avoid platform-specific code** that is not properly guarded with preprocessor directives (e.g., `#ifdef Q_OS_WIN`).
  * **Do not neglect performance.** Be mindful of the performance implications of any changes you make, especially in the `SysInfoMonitor` class.

-----

## How to Contribute

1.  **Analyze the request:** Understand the bug report or feature request.
2.  **Write the code:** Implement the necessary changes, following the existing code style.
3.  **Test your changes:** Thoroughly test your changes using the agent-based testing guidelines above.
4.  **Submit your contribution:** Provide a clear description of the changes you have made.

-----

## Key Files Overview

  * `overlaywidget.h`/`.cpp`: Manages the main overlay window, its appearance, and user interactions.
  * `sysinfomonitor.h`/`.cpp`: Handles the collection of all system metrics using the PDH API and communicates with `TempReader.exe`.
  * `settingsdialog.h`/`.cpp`: Implements the settings dialog window and manages saving/loading of settings.
  * `TempReader.csproj`/`TempReader.cs`: The C\# helper application for temperature monitoring.
  * `CMakeLists.txt`: The build script for the C++ application.
  * `.github/workflows/release.yml`: The GitHub Actions workflow for automated builds and releases.