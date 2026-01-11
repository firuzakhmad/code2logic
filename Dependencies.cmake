# X11 (Linux only, required by GLFW + ImGui)
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
endif()

# Dependencies Management
include(FetchContent)

# Option to use system packages when available
if(CODE2LOGIC_USE_SYSTEM_DEPS)
    message(STATUS "Using system dependencies where available")
    
    # Try to find system packages first
    find_package(glfw3 ${GLFW_VERSION} QUIET)
    find_package(glm ${GLM_VERSION} QUIET)
    find_package(OpenGL REQUIRED)
endif()

# Project directory structure
set(DEPS_DIR ${CMAKE_SOURCE_DIR}/dependencies)

# GLAD
if(NOT TARGET glad)
    message(STATUS "Using local GLAD")
    
    if(EXISTS ${DEPS_DIR}/glad/src/glad.c AND EXISTS ${DEPS_DIR}/glad/include/glad/glad.h)
        add_library(glad STATIC ${DEPS_DIR}/glad/src/glad.c)
        target_include_directories(glad PUBLIC ${DEPS_DIR}/glad/include)
        target_link_libraries(glad PUBLIC OpenGL::GL)
        
        # Set C standard for glad.c
        set_target_properties(glad PROPERTIES
            C_STANDARD 11
            C_STANDARD_REQUIRED ON
        )
        
        # Suppress warnings on Linux
        if(UNIX AND NOT APPLE)
            target_compile_options(glad PRIVATE -w)
        endif()
    else()
        message(WARNING "Local GLAD not found at ${DEPS_DIR}/glad")
        # Fallback to FetchContent if local copy is missing
        message(STATUS "Falling back to FetchContent for GLAD")
        FetchContent_Declare(
            glad
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            URL https://github.com/Dav1dde/glad/archive/refs/tags/v${GLAD_VERSION}.zip
        )
        FetchContent_MakeAvailable(glad)
        
        if(NOT TARGET glad)
            add_library(glad STATIC ${glad_SOURCE_DIR}/src/glad.c)
            target_include_directories(glad PUBLIC ${glad_SOURCE_DIR}/include)
            target_link_libraries(glad PUBLIC OpenGL::GL)
        endif()
    endif()
endif()

# STB
if(NOT TARGET stb)
    message(STATUS "Using local STB")
    
    # Check which STB libraries you have locally
    set(STB_SOURCES)
    set(STB_INCLUDES)
    
    # stb_image
    if(EXISTS ${DEPS_DIR}/stb_image/stb_image.cpp)
        list(APPEND STB_SOURCES ${DEPS_DIR}/stb_image/stb_image.cpp)
        list(APPEND STB_INCLUDES ${DEPS_DIR}/stb_image)
    endif()
    
    # stb_image_write
    if(EXISTS ${DEPS_DIR}/stb_image_write/stb_image_write.cpp)
        list(APPEND STB_SOURCES ${DEPS_DIR}/stb_image_write/stb_image_write.cpp)
        list(APPEND STB_INCLUDES ${DEPS_DIR}/stb_image_write)
    endif()
    
    # If we have any STB sources, create the library
    if(STB_SOURCES)
        add_library(stb STATIC ${STB_SOURCES})
        target_include_directories(stb PUBLIC ${STB_INCLUDES})
    else()
        message(WARNING "No local STB libraries found")
        # Fallback to FetchContent
        message(STATUS "Falling back to FetchContent for STB")
        FetchContent_Declare(
            stb
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            URL https://github.com/nothings/stb/archive/refs/heads/master.zip
        )
        FetchContent_MakeAvailable(stb)
        
        if(NOT TARGET stb)
            add_library(stb INTERFACE)
            target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})
        endif()
    endif()
endif()

# GLFW
if(NOT TARGET glfw)
    if(glfw3_FOUND AND CODE2LOGIC_USE_SYSTEM_DEPS)
        message(STATUS "Using system GLFW")
        add_library(glfw ALIAS glfw)
    else()
        message(STATUS "Fetching GLFW ${GLFW_VERSION}")
        FetchContent_Declare(
            glfw
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            URL https://github.com/glfw/glfw/releases/download/${GLFW_VERSION}/glfw-${GLFW_VERSION}.zip
        )
        FetchContent_MakeAvailable(glfw)
    endif()
endif()

# OpenGL
find_package(OpenGL REQUIRED)

# GLM
if(NOT TARGET glm)
    if(glm_FOUND AND CODE2LOGIC_USE_SYSTEM_DEPS)
        message(STATUS "Using system GLM")
        add_library(glm ALIAS glm)
    else()
        message(STATUS "Fetching GLM ${GLM_VERSION}")
        FetchContent_Declare(
            glm
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            URL https://github.com/g-truc/glm/archive/refs/tags/${GLM_VERSION}.zip
        )
        FetchContent_MakeAvailable(glm)
    endif()
endif()

# nlohmann/json
if(NOT TARGET nlohmann_json)
    message(STATUS "Fetching nlohmann_json ${NLOHMANN_JSON_VERSION}")
    FetchContent_Declare(
        nlohmann_json
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        URL https://github.com/nlohmann/json/archive/refs/tags/v${NLOHMANN_JSON_VERSION}.zip
    )
    FetchContent_MakeAvailable(nlohmann_json)
    
    # Create alias for consistency
    if(TARGET nlohmann_json::nlohmann_json AND NOT TARGET nlohmann_json)
        add_library(nlohmann_json ALIAS nlohmann_json::nlohmann_json)
    endif()
endif()

# Dear ImGui
if(CODE2LOGIC_BUILD_WITH_IMGUI AND NOT TARGET imgui)
    message(STATUS "Fetching ImGui (docking branch)")
    FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG docking
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(imgui)

    # Only create ImGui target if it doesn't exist
    if(NOT TARGET imgui)
        # Create ImGui library with platform backends
        add_library(imgui STATIC
                ${imgui_SOURCE_DIR}/imgui.cpp
                ${imgui_SOURCE_DIR}/imgui_demo.cpp
                ${imgui_SOURCE_DIR}/imgui_draw.cpp
                ${imgui_SOURCE_DIR}/imgui_tables.cpp
                ${imgui_SOURCE_DIR}/imgui_widgets.cpp
                ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
                ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        )

        target_include_directories(imgui PUBLIC
                ${imgui_SOURCE_DIR}
                ${imgui_SOURCE_DIR}/backends
        )

        if(UNIX AND NOT APPLE)
            target_link_libraries(imgui PUBLIC glfw OpenGL::GL glad X11::X11)
        else()
            target_link_libraries(imgui PUBLIC glfw OpenGL::GL glad)
        endif()
        target_compile_definitions(imgui PUBLIC
                IMGUI_IMPL_OPENGL_LOADER_GLAD
        )

        set_target_warnings(imgui)
    endif()
endif()

# Set folder properties for IDE organization
macro(set_dependency_folder target)
    if(TARGET ${target})
        set_target_properties(${target} PROPERTIES FOLDER "Dependencies")
    endif()
endmacro()

set_dependency_folder(glfw)
set_dependency_folder(glad)
set_dependency_folder(glm)
set_dependency_folder(stb)
set_dependency_folder(nlohmann_json)
set_dependency_folder(imgui)