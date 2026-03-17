<p>
  <img 
    src="https://github.com/firuzakhmad/code2logic/blob/main/resources/icons/default/code2logic/code2logic.png" 
    alt="Code2Logic Logo" width="180" 
  />
  <a href="LICENSE">
    <img src="https://img.shields.io/badge/License-MIT-blue.svg" alt="License"/>
  </a>
</p>

Code2Logic is a cross-platform algorithm visualization tool built with modern C++ and OpenGL. It provides interactive visualizations of common algorithms with detailed step-by-step execution, real-time variable tracking, and performance analysis.

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
    src="https://github.com/firuzakhmad/code2logic/blob/main/resources/screenshot/screenshot_3.png" 
    alt="Code2Logic" width="400" 
  />
</p>

## Getting Started

### Prerequisites
- **CMake** 3.16+
- **C++17** compatible compiler
- **OpenGL** 3.3+ support
- **Git**

### Installation

#### Windows
```bash
git clone https://github.com/firuzakhmad/code2logic.git
cd code2logic
mkdir build && cd build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

#### macOS
```bash
git clone https://github.com/firuzakhmad/code2logic.git
cd code2logic
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

#### Linux
```bash
git clone https://github.com/firuzakhmad/code2logic.git
cd code2logic
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```


### Running
After building, launch the application from:

    Windows: build/app/code2logic.exe

    macOS: build/app/code2logic.app

    Linux: build/app/code2logic

### Contributing

Contributions, ideas, and feedback are always welcome.

You can:
- Open an issue for bugs or feature requests
- Reach out directly via email for collaboration

**firuzakhmad.contribution@gmail.com**


### License

Code2Logic is released under the MIT License - see [LICENSE.txt](LICENSE.txt) for details.
