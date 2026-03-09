# Application Executable

set(CODE2LOGIC_SOURCES
        src/main.cpp
        src/core/application.cpp
        src/core/utils/logger/logger.cpp
        src/core/utils/timer/timer.cpp
        src/core/window/glfw_window.cpp
        src/core/input_handler/input_handler.cpp
        src/graphics/renderer.cpp
        src/scenes/base_scene.cpp
        src/scenes/scene_manager.cpp
        src/scenes/main_menu_scene.cpp
        src/graphics/imgui_manager.cpp
        src/core/file_system/std_file_system.cpp
        src/core/resources/font_resource.cpp
        src/core/resources/resource_manager.cpp
        src/ui/core/panel.cpp
        src/ui/core/popup.cpp
        src/ui/managers/ui_manager.cpp
        src/ui/components/main_menu.cpp
        src/ui/components/docking_layout.cpp
        src/core/resources/texture_resource.cpp
        src/core/resources/shader_resource.cpp
        src/algorithms/algorithm_base.cpp
        src/scenes/algorithm_visualizer_scene.cpp
        src/algorithms/bubble_sort.cpp
        src/algorithms/quick_sort.cpp
        src/algorithms/managers/algorithm_manager.cpp
        src/core/utils/thread_manager/thread_manager.cpp
        src/algorithms/array_based_visualizer.cpp
        src/ui/managers/icon_manager.cpp
        src/scenes/algorithm_comparison_scene.cpp
        src/algorithms/algorithm_variables.cpp
        src/core/json_config_manager/json_config_manager.cpp
        src/algorithms/algorithm_metadata_types.cpp
        src/algorithms/json_algorithm_base.cpp


)

set(CODE2LOGIC_HEADERS
        src/core/application.hpp
        src/core/utils/logger/logger.hpp
        src/core/utils/timer/i_timer.hpp
        src/core/utils/assert/assert.hpp
        src/core/utils/timer/timer.hpp
        src/core/window/window_settings.hpp
        src/core/window/i_window.hpp
        src/core/window/glfw_window.hpp
        src/core/input_handler/input_handler.hpp
        src/graphics/renderer.hpp
        src/scenes/i_scene.hpp
        src/scenes/base_scene.hpp
        src/scenes/scene_manager.hpp
        src/scenes/main_menu_scene.hpp
        src/graphics/imgui_manager.hpp
        src/core/file_system/i_file_system.hpp
        src/core/file_system/std_file_system.hpp
        src/core/resources/i_resource.hpp
        src/core/resources/font_loader.hpp
        src/core/resources/font_resource.hpp
        src/core/resources/i_resource_loader.hpp
        src/core/resources/resource_manager.hpp
        src/ui/core/i_ui_component.hpp
        src/ui/core/panel.hpp
        src/ui/core/popup.hpp
        src/ui/core/widget.hpp
        src/ui/managers/ui_manager.hpp
        src/ui/components/main_menu.hpp
        src/ui/components/docking_layout.hpp
        src/core/resources/texture_resource.hpp
        src/core/resources/texture_loader.hpp
        src/core/resources/shader_resource.hpp
        src/core/resources/shader_loader.hpp
        src/core/resources/shader_types.hpp
        src/scenes/algorithm_visualizer_scene.hpp
        src/algorithms/i_simple_algorithm.hpp
        src/algorithms/algorithm_observer.hpp
        src/algorithms/algorithm_step.hpp
        src/algorithms/algorithm_step_operation.hpp
        src/algorithms/algorithm_state_tracker.hpp
        src/algorithms/algorithm_base.hpp
        src/algorithms/bubble_sort.hpp
        src/algorithms/quick_sort.hpp
        src/algorithms/managers/algorithm_manager.hpp
        src/core/utils/thread_manager/thread_manager.hpp
        src/algorithms/i_algorithm_visualizer.hpp
        src/algorithms/code_highlight.hpp
        src/algorithms/array_based_visualizer.hpp
        src/ui/managers/icon_manager.hpp
        src/scenes/algorithm_comparison_scene.hpp
        src/algorithms/algorithm_variable.hpp
        src/core/json_config_manager/json_config_manager.hpp
        src/core/utils/utils.hpp
        src/core/utils/variables.hpp
        src/algorithms/i_algorithm_metadata.hpp
        src/algorithms/algorithm_metadata_types.hpp
        src/algorithms/json_algorithm_base.hpp

)

add_executable(code2logic
        ${CODE2LOGIC_SOURCES}
        ${CODE2LOGIC_HEADERS}
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${CODE2LOGIC_SOURCES} ${CODE2LOGIC_HEADERS})

set_target_warnings(code2logic)

target_link_libraries(code2logic PRIVATE
        glfw
        OpenGL::GL
        glad
        glm
        stb
        nlohmann_json
        imgui
)

target_include_directories(code2logic PRIVATE src)

# Platform-specific executable properties
if(WIN32)
    set_target_properties(code2logic PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
else()
    set_target_properties(code2logic PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()

# Setting folder property for IDE organization
set_target_properties(code2logic PROPERTIES FOLDER "app")