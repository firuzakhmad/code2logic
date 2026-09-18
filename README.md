<p>
  <img 
    src="https://github.com/firuzakhmad/code2logic/blob/main/resources/icons/default/code2logic/code2logic.png" 
    alt="Code2Logic Logo" width="180" 
  />
  <a href="https://github.com/firuzakhmad/code2logic/actions/workflows/ci.yml">
    <img src="https://github.com/firuzakhmad/code2logic/actions/workflows/ci.yml/badge.svg" alt="Build Status"/>
  </a>
  <a href="LICENSE.txt">
    <img src="https://img.shields.io/badge/License-MIT-blue.svg" alt="License"/>
  </a>
</p>

Code2Logic is a cross-platform algorithm visualization tool built with modern C++ and OpenGL. It provides interactive visualizations of common algorithms with detailed step-by-step execution, real-time variable tracking, and performance analysis.

## Download

Prebuilt binaries for Windows, macOS, and Linux are published automatically
with every release — no compiler or dependencies needed:

**[Latest Release](https://github.com/firuzakhmad/code2logic/releases/latest)**

If you'd rather build from source, see [Getting Started](#getting-started) below.

## Features

### Algorithm Visualization
- Interactive step-by-step execution with forward/backward navigation
- Multiple visualization styles including bar charts, dot plots, network graphs, and tree visualizations
- Real-time variable tracking and state monitoring
- Visual comparison of elements during algorithm execution
<p>
  <img 
    src="https://github.com/firuzakhmad/code2logic/blob/main/resources/screenshot/screenshot_1.png" 
    alt="Code2Logic" width="400" 
  />
  <img 
    src="https://github.com/firuzakhmad/code2logic/blob/main/resources/screenshot/screenshot_2.png" 
    alt="Code2Logic" width="400" 
  />
</p>
<p>
  <img 
    src="https://github.com/firuzakhmad/code2logic/blob/main/resources/screenshot/screenshot_3.png" 
    alt="Code2Logic" width="400" 
  />
  <img 
    src="https://github.com/firuzakhmad/code2logic/blob/main/resources/screenshot/screenshot_4.png" 
    alt="Code2Logic" width="400" 
  />
</p>
<p>
  <img 
    src="https://github.com/firuzakhmad/code2logic/blob/main/resources/screenshot/screenshot_5.png" 
    alt="Code2Logic" width="400" 
  />
</p>

## Getting Started

### Prerequisites
- **CMake** 3.16+
- **C++20** compatible compiler
- **OpenGL** 3.3+ support
- **Git**

### Installation

#### Windows
```bash
git clone https://github.com/firuzakhmad/code2logic.git
cd code2logic
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

#### macOS
```bash
git clone https://github.com/firuzakhmad/code2logic.git
cd code2logic
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

#### Linux
On Debian/Ubuntu, install the required OpenGL and X11 development packages:
```bash
sudo apt update
sudo apt install libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev xorg-dev
```
Then clone and build Code2Logic:
```bash
git clone https://github.com/firuzakhmad/code2logic.git
cd code2logic
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```
Package names may differ on other Linux distributions.



### Running
After building, launch the application from:

    Windows: build/bin/code2logic.exe
    macOS:   build/bin/code2logic.app
    Linux:   build/bin/code2logic

### Contributing

Contributions, ideas, and feedback are always welcome.

You can:
- Open an issue for bugs or feature requests


### License

Code2Logic is released under the MIT License - see [LICENSE.txt](LICENSE.txt) for details.
