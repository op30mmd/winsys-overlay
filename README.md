# WinSys-Overlay

A comprehensive, lightweight, always-on-top system resource monitor for Windows, built with C++ and Qt. Monitor your system's performance with a customizable overlay that stays visible over all applications.

## Features

### 🖥️ Comprehensive System Monitoring
*   **CPU Load (%)**: Real-time processor utilization
*   **Memory Usage (%)**: System memory utilization percentage
*   **Detailed RAM Usage**: Used/Total memory in MB
*   **Physical Disk Activity (%)**: Disk I/O utilization
*   **GPU Load (%)**: Highest utilization across all GPU engines
*   **FPS Estimation**: Estimated frame rate based on system performance
*   **Network Activity**: Real-time download and upload speeds (MB/s)
*   **Daily Data Usage**: Internet usage tracking in MB per day
*   **CPU Temperature**: Processor temperature monitoring (when available)
*   **GPU Temperature**: Graphics card temperature (vendor-specific)
*   **Active Processes**: Count of running system processes
*   **System Uptime**: How long the system has been running

### 🌡️ Temperature Monitoring
*   **CPU Temperature**: Monitors the temperature of the CPU.
*   **GPU Temperature**: Monitors the temperature of the GPU.
*   **Vendor Agnostic**: Uses LibreHardwareMonitor to support a wide range of hardware (Intel, AMD, NVIDIA).

### 🎨 Highly Customizable Interface
*   **Layout Options**: Choose between vertical or horizontal layout orientations
*   **Font Customization**: Adjustable font size (8-24px) and color
*   **Background Styling**: Customizable background color and opacity (0-255)
*   **Selective Display**: Show/hide individual metrics as needed
*   **Icon Integration**: Each metric displays with a distinctive icon
*   **Visual Effects**: Text includes subtle drop shadows for readability

### ⚙️ Advanced Behavior Controls
*   **Update Frequency**: Configurable refresh interval (250ms - 5000ms)
*   **Persistent Settings**: Saves your preferences and window position automatically
*   **Smart Daily Tracking**: Network usage resets daily with 30-day history cleanup
*   **Performance Optimized**: Efficient Windows PDH API integration

### 🖱️ User-Friendly Interface
*   **Always-On-Top**: Stays visible over all other windows, including Start Menu and Task Manager
*   **Frameless Design**: Clean, minimalist interface that doesn't obstruct your workflow
*   **Draggable**: Click and drag to position anywhere on your screen
*   **Context Menu**: Right-click for quick access to settings and exit options
*   **Scrollable Settings**: Organized settings dialog with grouped options

## Usage

### Basic Controls
*   **Move Overlay**: Left-click and drag to reposition the overlay anywhere on screen
*   **Access Settings**: Right-click the overlay to open the context menu
*   **Customize Display**: Use the Settings dialog to configure which metrics are shown
*   **Adjust Appearance**: Modify colors, fonts, opacity, and layout orientation

### Settings Categories

#### 🎨 Appearance
- Layout orientation (Vertical/Horizontal)
- Font size and color selection
- Background color and opacity control

#### 📊 Displayed Information
Toggle visibility for each metric:
- Core metrics: CPU, Memory, RAM, Disk, GPU (enabled by default)
- Extended metrics: FPS, Network speeds, Daily usage, Temperatures, Processes, Uptime (disabled by default)

#### ⚙️ Behavior
- Update interval configuration
- Performance optimization settings

---

## Building from Source

### Prerequisites

1.  **Visual Studio**: [Download](https://visualstudio.microsoft.com/vs/community/) with "Desktop development with C++" and ".NET desktop development" workloads.
2.  **.NET SDK**: [Download](https://dotnet.microsoft.com/download) (version 6.0 or later).
3.  **CMake**: Download from [cmake.org](https://cmake.org/download/)
4.  **Qt 6**: Download the [Qt Online Installer](https://www.qt.io/download-qt-installer)
    - Select Qt 6.x version for MSVC 2019/2022 64-bit
    - Ensure **Qt Widgets** module is included

### Build Steps

1.  **Clone the repository:**
    ```bash
    git clone https://github.com/op30mmd/winsys-overlay.git
    cd winsys-overlay
    ```

2.  **Create build directory:**
    ```bash
    mkdir build
    cd build
    ```

3.  **Configure with CMake:**
    ```bash
    # Replace with your actual Qt installation path
    cmake .. -DCMAKE_PREFIX_PATH=C:\Qt\6.9.1\msvc2022_64
    ```

4.  **Build the C# Temperature Reader:**
    ```bash
    cd ../ # Return to the root directory
    dotnet publish TempReader.csproj -c Release -r win-x64 --self-contained false
    ```
    This will create a `TempReader.exe` in `bin/Release/net6.0/win-x64`.

5.  **Copy Dependencies:**
    - Copy the compiled `TempReader.exe` to the `build/Release` directory.
    - Copy `libs/lhm/LibreHardwareMonitorLib.dll` to the `build/Release` directory.

6.  **Build the main application:**
    ```bash
    cd build # Return to the build directory
    cmake --build . --config Release
    ```

### Automated Builds

The project includes GitHub Actions workflow for automated building:
- Supports self-hosted Windows runners
- Automatic dependency resolution with windeployqt
- Optional UPX compression for smaller executables
- Automated releases on version tags

## Technical Implementation

### System Monitoring Architecture
- **Hybrid C++/C# Approach**: The core application is built with C++ and Qt for performance and a native feel, while temperature monitoring is handled by a separate C# helper process.
- **LibreHardwareMonitor Integration**: The C# `TempReader.exe` utility uses the `LibreHardwareMonitorLib.dll` to query CPU and GPU temperatures, supporting a wide range of hardware from vendors like Intel, AMD, and NVIDIA.
- **Inter-Process Communication**: The main C++ application launches `TempReader.exe` in the background, capturing its standard output to retrieve temperature data. This isolates the .NET environment from the main application, minimizing dependencies.
- **Windows PDH API**: Native Performance Data Helper for efficient system metrics for all other data points.
- **Multi-Query Design**: Separate PDH queries for CPU, Disk, GPU, Network, and Temperature monitoring
- **Smart Caching**: Optimized data collection to minimize system impact
- **Wildcard Counter Expansion**: Automatically detects available GPU engines and network interfaces

### Data Persistence
- **QSettings Integration**: Cross-platform settings storage
- **Daily Usage Tracking**: Persistent network usage with automatic cleanup
- **Position Memory**: Remembers overlay position between sessions

### Windows Integration
- **Always-On-Top Enforcement**: Periodic re-assertion of topmost window status
- **Transparent Background**: Native transparency support with customizable opacity
- **System API Integration**: Direct Windows API calls for enhanced functionality

## Performance Considerations

The overlay is designed to be lightweight and efficient:
- **Minimal CPU Impact**: Optimized update cycles and efficient API usage
- **Memory Efficient**: Smart counter management and automatic cleanup
- **Configurable Updates**: Adjustable refresh rates to balance accuracy and performance
- **GPU Monitoring**: Non-intrusive GPU utilization tracking across all engines

## Contributing

Contributions are welcome! Areas for potential enhancement:
- Additional system metrics
- Theme and styling improvements
- Cross-platform support expansion
- Performance optimizations

Please feel free to open issues for bug reports or feature requests, and submit pull requests for improvements.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.

## System Requirements

- **OS**: Windows 10/11 (64-bit recommended)
- **Runtime**: Visual C++ Redistributable 2019/2022
- **Memory**: Minimal system impact (< 50MB RAM)
- **Permissions**: Standard user privileges (no administrator rights required)