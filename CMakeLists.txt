cmake_minimum_required(VERSION 3.15)

project(Code2Logic VERSION 1.0.0 LANGUAGES C CXX)

# Include versions
include(Versions.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json for tools like clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

# Build configuration options
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(CODE2LOGIC_BUILD_TESTS "Build tests" OFF)
option(CODE2LOGIC_BUILD_EXAMPLES "Build examples" OFF)
option(CODE2LOGIC_BUILD_WITH_IMGUI "Build with ImGui support" ON)
option(CODE2LOGIC_BUILD_WITH_ECS "Build with ECS support" ON)
option(CODE2LOGIC_USE_SYSTEM_DEPS "Use system-installed dependencies" OFF)

# Platform detection and configuration
include(CMake/CompilerWarnings.cmake)

if(WIN32)
    message(STATUS "Platform: Windows")
    add_compile_definitions(_WIN32_WINNT=0x0601 NOMINMAX)
elseif(APPLE)
    message(STATUS "Platform: macOS")
    set(CMAKE_MACOSX_RPATH ON)
elseif(UNIX AND NOT APPLE)
    message(STATUS "Platform: Linux/Unix")
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
endif()

# Print configuration summary
message(STATUS "Code2Logic Version: ${CODE2LOGIC_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Use system deps: ${CODE2LOGIC_USE_SYSTEM_DEPS}")

# Dependencies
include(Dependencies.cmake)

add_subdirectory(app)

install(
    DIRECTORY 
    DESTINATION include
    COMPONENT Code2Logic_Development
)

install(
    TARGETS code2logic
    EXPORT Code2LogicTargets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    BUNDLE DESTINATION .
    COMPONENT Code2Logic_Runtime
)

install(
    EXPORT Code2LogicTargets
    DESTINATION lib/cmake/Code2Logic
    NAMESPACE c2l::
    COMPONENT Code2Logic_Development
)