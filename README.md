# WinSys-Overlay

A simple, lightweight, always-on-top system resource monitor for Windows, built with C++ and Qt.



## Features

*   **Comprehensive Monitoring**: Real-time display of key system metrics:
    *   CPU Load (%)
    *   Memory Usage (%)
    *   Detailed RAM Usage (Used/Total MB)
    *   Physical Disk Activity (%)
    *   GPU Load (highest utilization across all engines)
*   **Always-On-Top**: Stays visible over all other windows, including the Windows Start Menu and Task Manager.
*   **Minimalist UI**: A clean, frameless, and transparent interface that doesn't get in your way.
*   **Draggable**: Simply click and drag the overlay to position it anywhere on your screen.
*   **High Readability**: Text includes a subtle drop shadow, ensuring it's easy to read against any background, light or dark.

## Usage

*   **Move**: Click and drag with the left mouse button to move the overlay.
*   **Close**: Right-click the overlay and select "Close" to exit the application.

---

## Building from Source

To build the project yourself, you will need to have a few tools installed and follow the steps below.

### Prerequisites

1.  **A C++ Compiler**: [Visual Studio](https://visualstudio.microsoft.com/vs/community/) (Community, Professional, or Enterprise) with the "Desktop development with C++" workload installed.
2.  **CMake**: Download and install from the [official CMake website](https://cmake.org/download/).
3.  **Qt 6**: Download the [Qt Online Installer](https://www.qt.io/download-qt-installer). During installation, make sure to select a Qt 6 version (e.g., 6.5.0) for your compiler (e.g., MSVC 2019/2022 64-bit) and ensure the **Qt Widgets** module is included.

### Build Steps

1.  **Clone the repository:**
    ```sh
    git clone https://github.com/op30mmd/winsys-overlay.git
    cd winsys-overlay
    ```

2.  **Create a build directory:**
    ```sh
    mkdir build
    cd build
    ```

3.  **Configure the project with CMake.**
    
    You must tell CMake where your Qt installation is located. Replace the path in the command below with the actual path to your Qt version.

    ```sh
    # Example path for Qt 6.5.0 installed for MSVC 2019 64-bit
    cmake .. -DCMAKE_PREFIX_PATH=C:\Qt\6.5.0\msvc2019_64
    ```

4.  **Build the application:**
    ```sh
    cmake --build . --config Release
    ```

## Running the Application

After a successful build, the executable (`winsys-overlay.exe`) and all required Qt DLLs will be located in the `build\Release` directory.

You can run the application by double-clicking the executable or by running it from the command line:

```sh
.\Release\winsys-overlay.exe
```

## How It Works

The application uses the native Windows Performance Data Helper (PDH) API to query system information counters for CPU, Disk, and GPU utilization. Memory information is gathered using the `GlobalMemoryStatusEx` function.

To ensure the overlay remains on top of system UI like the Start Menu, it periodically re-asserts its "topmost" window status using the `SetWindowPos` Win32 API call with the `HWND_TOPMOST` and `SWP_NOACTIVATE` flags.

## Contributing

Contributions are welcome! If you have ideas for new features or find a bug, feel free to open an issue or submit a pull request.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.
