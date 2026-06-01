#include "scenes/algorithm_comparison_scene.hpp"

#include <cinttypes>

#include "core/utils/logger/logger.hpp"
#include "core/utils/variables.hpp"
#include "algorithms/visualizers/grid_cell_data.hpp"
#include "algorithms/visualizers/grid_cell_type.hpp"


#include "imgui.h"
#include "algorithms/core/grid_pathfinding_base.hpp"
#include "algorithms/visualizers/graph_based_visualizer.hpp"

namespace c2l::scenes
{
    AlgorithmComparisonScene::AlgorithmComparisonScene(
        graphics::Renderer& renderer,
        core::ThreadManager& thread_manager,
        core::JsonConfigManager& json_config_manager,
        algorithms::AlgorithmRegistry& algorithm_registry,
        core::resources::ResourceManager& resource_manager,
        ui::managers::IconManager& icon_manager)
        : BaseScene{
            renderer,
            thread_manager,
            json_config_manager,
            algorithm_registry,
            resource_manager,
            icon_manager
        }
        , m_comparison_manager{
            std::make_unique<algorithms::ParallelComparisonManager>(
                thread_manager,
                algorithm_registry)
        }
    {
        LOG_INFO("AlgorithmComparisonScene created");
    }

    void AlgorithmComparisonScene::on_create()
    {
        setup_ui_components();
        setup_main_menu();

        m_cached_categorized_algorithms = m_algorithm_registry
            .get_available_categorized_algorithms();

        // Initialize with default algorithms
        m_comparison_manager->set_algorithm_left(m_left_selected);
        m_comparison_manager->set_algorithm_right(m_right_selected);
        m_comparison_manager->generate_random_data(
            static_cast<size_t>(m_data_size),
            m_max_value
        );

        // Initializing visualizer
        sync_visualizers();

        LOG_INFO("AlgorithmComparisonScene initialized");
    }

    void AlgorithmComparisonScene::on_destroy()
    {
        m_ui_manager->unregister_all_components();
        LOG_DEBUG("AlgorithmComparisonScene destroyed");
    }

    void AlgorithmComparisonScene::on_activate()
    {
        if (!is_created())
        {
            on_create();
            mark_created();
        }

        sync_visualizers();
    }

    void AlgorithmComparisonScene::on_deactivate()
    {
        m_ui_manager->hide_all_panels();
        m_ui_manager->close_all_popups();

        if (m_comparison_manager)
        {
            m_comparison_manager->pause();
        }
    }

    void AlgorithmComparisonScene::process_input(const core::InputHandler& input)
    {
        // Playback controls
        if (input.is_key_just_pressed(GLFW_KEY_SPACE))
        {
            if (m_comparison_manager->is_playing())
                m_comparison_manager->pause();
            else
                m_comparison_manager->play();
        }

        if (input.is_key_just_pressed(GLFW_KEY_RIGHT))
        {
            m_comparison_manager->step_forward();
        }

        if (input.is_key_just_pressed(GLFW_KEY_LEFT))
        {
            m_comparison_manager->step_backward();
        }

        if (input.is_key_just_pressed(GLFW_KEY_R))
        {
            m_comparison_manager->reset();
        }

        if (input.is_key_just_pressed(GLFW_KEY_N))
        {
            m_comparison_manager->generate_random_data(
                static_cast<size_t>(m_data_size),
                m_max_value
            );
        }

        // Exit
        if (input.is_key_just_pressed(GLFW_KEY_ESCAPE))
        {
            request_scene_pop();
        }
    }

    void AlgorithmComparisonScene::update(double dt)
    {
        m_ui_manager->update(dt);
        m_animation_time += dt;

        if (m_comparison_manager)
        {
            m_visualizer.update(dt);
        }

        m_icon_manager.update();
    }

    void AlgorithmComparisonScene::render()
    {
        m_renderer.render();
        m_ui_manager->render();

        // Render all panels
        if (m_show_selection_panel)
            render_selection_panel();

        if (m_show_comparison_view)
            render_comparison_view_panel();

        if (m_show_control_panel)
            render_control_panel();

        if (m_show_data_control)
            render_data_control_panel();

        if (m_show_results_panel)
            render_results_panel();

        if (m_show_performance_graphs)
            render_performance_graphs();

        render_common_ui();
        m_renderer.clear();
    }

    void AlgorithmComparisonScene::render_selection_panel()
    {
        ImGui::SetNextWindowSize(DEFAULT_WINDOW_SIZE, ImGuiCond_FirstUseEver);

        if (!ImGui::Begin("Algorithm Selection", &m_show_selection_panel, ImGuiWindowFlags_NoCollapse))
        {
            ImGui::End();
            return;
        }

        // Category Selection
        ImGui::TextColored(ImVec4(1.0f, 0.984f, 0.0f, 1.0f), "Algorithm Categories");
        ImGui::Separator();

        if (m_cache_dirty)
        {
            rebuild_category_cache();
        }

        if (ImGui::BeginCombo("##algorithm_categories", m_selected_category.c_str()))
        {
            for (const auto& category_name : m_cached_category_names)
            {
                bool is_selected = (m_selected_category == category_name);
                if (ImGui::Selectable(category_name.c_str(), is_selected))
                {
                    m_selected_category = category_name;
                    m_current_display_algorithms = m_cached_categorized_algorithms[category_name];

                    rebuild_category_cache();
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Left and Right Algorithm Selection - Side by Side
        ImGui::BeginTable("algorithm_selection_table", 2, ImGuiTableFlags_SizingStretchSame);

        // Column 1: Left Algorithm
        ImGui::TableNextColumn();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Left Algorithm");
        ImGui::Separator();

        // Left algorithm combo
        if (ImGui::BeginCombo("##LeftAlgorithm", algorithm_display_name(m_left_selected).data()))
        {
            for (const auto* algo : m_current_display_algorithms)
            {
                if (!algo) continue;

                bool is_selected = (m_left_selected == algo->type);
                if (ImGui::Selectable(algo->display_name.data(), is_selected))
                {
                    m_left_selected = algo->type;
                    m_comparison_manager->set_algorithm_left(algo->type);
                    sync_visualizers();
                }
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        // Column 2: Right Algorithm-
        ImGui::TableNextColumn();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Right Algorithm");
        ImGui::Separator();

        // Right algorithm combo
        if (ImGui::BeginCombo("##RightAlgorithm", algorithm_display_name(m_right_selected).data()))
        {
            for (const auto* algo : m_current_display_algorithms)
            {
                if (!algo) continue;

                bool is_selected = (m_right_selected == algo->type);
                if (ImGui::Selectable(algo->display_name.data(), is_selected))
                {
                    m_right_selected = algo->type;
                    m_comparison_manager->set_algorithm_right(algo->type);
                    sync_visualizers();
                }
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::EndTable();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Swap button
        if (ImGui::Button("Swap Algorithms", ImVec2(-1, 0)))
        {
            std::swap(m_left_selected, m_right_selected);
            m_comparison_manager->set_algorithm_left(m_left_selected);
            m_comparison_manager->set_algorithm_right(m_right_selected);
            sync_visualizers();
        }

        ImGui::End();
    }

    void AlgorithmComparisonScene::render_comparison_view_panel()
    {
        ImGui::SetNextWindowSize(DEFAULT_WINDOW_SIZE, ImGuiCond_FirstUseEver);

        ImGui::Begin("Comparison View", &m_show_comparison_view, ImGuiWindowFlags_NoScrollbar);

        // Render side-by-side visualization
        m_visualizer.render();

        ImGui::End();
    }

    void AlgorithmComparisonScene::render_control_panel()
    {
        ImGui::SetNextWindowSize(
            DEFAULT_WINDOW_SIZE, 
            ImGuiCond_FirstUseEver
        );

        if (!ImGui::Begin("Comparison Controls", &m_show_control_panel))
        {
            ImGui::End();
            return;
        }

        const bool is_playing = m_comparison_manager->is_playing();

        // Playback controls
        ImGui::TextUnformatted("Playback");
        ImGui::Separator();

        if (ImGui::BeginTable(
            "PlaybackControls",
            3,
            ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            if (ImGui::Button(
                is_playing ?  "Pause" : "Play",
                ImVec2(-1, DEFAULT_BUTTON_HEIGHT))
                )
            {
                is_playing ? m_comparison_manager->pause()
                           : m_comparison_manager->play();
            }

            ImGui::TableSetColumnIndex(1);
            if (ImGui::Button(
                "Stop",
                ImVec2(-1, DEFAULT_BUTTON_HEIGHT))
                )
            {
                m_comparison_manager->stop();
            }

            ImGui::TableSetColumnIndex(2);
            if (ImGui::Button("Reset", ImVec2(-1, DEFAULT_BUTTON_HEIGHT)))
            {
                m_comparison_manager->reset();
            }

            ImGui::EndTable();
        }

        // Step Controls
        ImGui::Spacing();
        ImGui::TextUnformatted("Stepping");
        ImGui::Separator();

        const bool can_step = !is_playing;

        if (ImGui::BeginTable(
            "StepControls",
            2,
            ImGuiTableFlags_SizingStretchSame)
            )
        {
            ImGui::TableNextRow();

            ImGui::BeginDisabled(!can_step);

            ImGui::TableSetColumnIndex(0);
            if (ImGui::Button(
                "Step Back",
                ImVec2(-1, DEFAULT_BUTTON_HEIGHT))
                )
            {
                m_comparison_manager->step_backward();
            }

            ImGui::TableSetColumnIndex(1);
            if (ImGui::Button(
                "Step Forward",
                ImVec2(-1, DEFAULT_BUTTON_HEIGHT))
                )
            {
                m_comparison_manager->step_forward();
            }

            ImGui::EndDisabled();

            ImGui::EndTable();
        }

        // Speed control
        ImGui::Spacing();
        ImGui::TextUnformatted("Speed");
        ImGui::Separator();

        float speed = m_comparison_manager->get_speed();

        ImGui::SetNextItemWidth(-1);
        if (ImGui::SliderFloat(
            "##Speed",
            &speed,
            ALGORITHM_COMPUTATION_MIN_SPEED,
            ALGORITHM_COMPUTATION_MAX_SPEED,
            "%.1fx"))
        {
            m_comparison_manager->set_speed(speed);
        }

        // Progress bars
        ImGui::Spacing();
        ImGui::TextUnformatted("Progress");
        ImGui::Separator();

        const auto& left = m_comparison_manager->get_left_algorithm();
        const auto& right = m_comparison_manager->get_right_algorithm();

        if (left.is_valid())
        {
            ImGui::ProgressBar(left.metrics.progress,
                              ImVec2(-1, PROGRESS_BAR_HEIGHT),
                              left.name.c_str());
        }

        if (right.is_valid())
        {
            ImGui::ProgressBar(right.metrics.progress,
                              ImVec2(-1, PROGRESS_BAR_HEIGHT),
                              right.name.c_str());
        }

        ImGui::End();
    }

    void AlgorithmComparisonScene::render_data_control_panel()
    {
        ImGui::SetNextWindowSize(
            DEFAULT_WINDOW_SIZE, 
            ImGuiCond_FirstUseEver
        );

        if (!ImGui::Begin("Data Controls", &m_show_data_control))
        {
            ImGui::End();
            return;
        }

        switch (m_selected_visualization_type)
        {
            case algorithms::VisualizationType::ARRAY_BASED_VISUALIZATION:
                render_array_based_data_controls();
                break;
            case algorithms::VisualizationType::GRAPH_BASED_VISUALIZATION:
                render_graph_based_data_controls();
                break;
            case algorithms::VisualizationType::PATH_FINDING_BASED_VISUALIZATION:
                render_path_based_data_controls();
                break;

            default:
                render_array_based_data_controls();
                break;
        }

        ImGui::End();
    }

    void AlgorithmComparisonScene::render_array_based_data_controls()
    {
                // Data Size
        ImGui::TextUnformatted("Data Size");
        ImGui::Separator();

        ImGui::SetNextItemWidth(-1);
        // Data size slider
        ImGui::SliderInt(
            "##Data Size",
            &m_data_size,
            ALGORITHM_DATA_MIN_SIZE,
            ALGORITHM_DATA_MAX_SIZE
        );

        ImGui::Spacing();
        ImGui::TextUnformatted("Max Value");
        ImGui::Separator();

        ImGui::SetNextItemWidth(-1);
        ImGui::SliderInt(
            "##Max Value",
            &m_max_value,
            ALGORITHM_DATA_MIN_VALUE,
            ALGORITHM_DATA_MAX_VALUE
        );

        // Preset data sets
        ImGui::Spacing();
        ImGui::TextUnformatted("Presets:");
        ImGui::Separator();

        if (ImGui::BeginTable(
            "Presets",
            3,
            ImGuiTableFlags_SizingStretchSame)
            )
        {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            if (ImGui::Button(
                "Sorted",
                ImVec2(-1, DEFAULT_BUTTON_HEIGHT))
                )
            {
                std::vector<int> data;
                for (int i = 1; i <= m_data_size; ++i)
                {
                    data.push_back(i);
                }
                m_comparison_manager->set_shared_data(data);
            }

            ImGui::TableSetColumnIndex(1);
            if (ImGui::Button(
                "Reverse",
                ImVec2(-1, DEFAULT_BUTTON_HEIGHT))
                )
            {
                std::vector<int> data;
                for (int i = m_data_size; i >= 1; --i)
                {
                    data.push_back(i);
                }
                m_comparison_manager->set_shared_data(data);
            }

            ImGui::TableSetColumnIndex(2);
            if (ImGui::Button(
                "Constant",
                ImVec2(-1, DEFAULT_BUTTON_HEIGHT))
                )
            {
                std::vector<int> data(
                    static_cast<size_t>(m_data_size),
                    42
                );
                m_comparison_manager->set_shared_data(data);
            }

            ImGui::EndTable();
        }

        ImGui::Dummy(ImVec2(0.0f, 20.0f));
        if (ImGui::Button(
            "Generate Random Data",
            ImVec2(-1, DEFAULT_BUTTON_HEIGHT))
            )
        {
            m_comparison_manager->generate_random_data(
                static_cast<size_t>(m_data_size),
                m_max_value
            );
        }
    }

    void AlgorithmComparisonScene::render_path_based_data_controls()
    {
        render_shared_grid_editor();
    }

    void AlgorithmComparisonScene::render_shared_grid_editor()
    {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Shared Grid Editor");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                           "Changes apply to BOTH algorithms simultaneously");
        ImGui::Separator();

        // Tool selection - using TOOL_NAMES from grid_cell_data.hpp
        ImGui::Text("Tools");
        int current_tool = static_cast<int>(m_current_grid_tool);
        for (int i = 0; i < IM_ARRAYSIZE(algorithms::TOOL_NAMES); ++i)
        {
            ImGui::PushID(i);
            if (ImGui::Selectable(algorithms::TOOL_NAMES[i], current_tool == i))
            {
                m_current_grid_tool = static_cast<algorithms::GridToolMode>(i);
            }
            ImGui::PopID();
        }

        ImGui::Dummy({0, 10});
        ImGui::Separator();
        ImGui::Dummy({0, 10});

        // Grid size controls
        ImGui::Text("Grid Configuration");
        ImGui::SetNextItemWidth(-1);

        int grid_size[2] = {m_shared_grid_state.rows, m_shared_grid_state.cols};
        if (ImGui::InputInt2("Size (Rows, Cols)", grid_size))
        {
            grid_size[0] = std::clamp(grid_size[0], 5, 50);
            grid_size[1] = std::clamp(grid_size[1], 5, 50);
            m_shared_grid_state.resize(grid_size[0], grid_size[1]);
            m_shared_grid_state.set_start(5, 5);
            m_shared_grid_state.set_target(grid_size[0] - 5, grid_size[1] - 5);
            sync_shared_grid_to_algorithms();
        }

        if (ImGui::Button("Clear Grid", ImVec2(-1, 0)))
        {
            m_shared_grid_state.clear();
            m_shared_grid_state.set_start(5, 5);
            m_shared_grid_state.set_target(m_shared_grid_state.rows - 5, m_shared_grid_state.cols - 5);
            sync_shared_grid_to_algorithms();
        }

        if (ImGui::Button("Random Maze", ImVec2(-1, 0)))
        {
            m_shared_grid_state.generate_random_maze(0.35f);
            sync_shared_grid_to_algorithms();
        }

        if (ImGui::Button("Recursive Maze", ImVec2(-1, 0)))
        {
            m_shared_grid_state.generate_recursive_backtracking_maze();
            sync_shared_grid_to_algorithms();
        }

        ImGui::Separator();

        // Display options
        ImGui::Text("Display");
        ImGui::Checkbox("Grid Lines", &m_show_grid_lines);
        ImGui::Checkbox("Show Weights", &m_show_weights);
        ImGui::Checkbox("Show Coordinates", &m_show_coordinates);

        ImGui::Separator();

        // Mini grid preview
        render_mini_grid_preview();

        ImGui::Separator();

        // Grid info
        render_shared_grid_info();

        ImGui::Separator();

        // Legend
        render_shared_grid_legend();
    }

    void AlgorithmComparisonScene::render_mini_grid_preview()
    {
        ImGui::Text("Grid Preview");

        float preview_size = 250.0f;
        float cell_size = preview_size / std::max(m_shared_grid_state.cols, m_shared_grid_state.rows);
        float preview_width = cell_size * m_shared_grid_state.cols;
        float preview_height = cell_size * m_shared_grid_state.rows;

        ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
        ImVec2 grid_offset(cursor_pos.x + (preview_size - preview_width) * 0.5f,
                           cursor_pos.y + (preview_size - preview_height) * 0.5f);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Draw preview background
        draw_list->AddRectFilled(grid_offset,
            ImVec2(grid_offset.x + preview_width, grid_offset.y + preview_height),
            ImColor(25, 30, 40, 255));

        // Draw cells using get_cell_color_for_preview from grid_cell_data.hpp
        for (int row = 0; row < m_shared_grid_state.rows; ++row)
        {
            for (int col = 0; col < m_shared_grid_state.cols; ++col)
            {
                ImVec2 cell_pos(grid_offset.x + col * cell_size, grid_offset.y + row * cell_size);
                ImColor color = algorithms::get_cell_color_for_preview(m_shared_grid_state.grid[row][col]);
                draw_list->AddRectFilled(cell_pos,
                    ImVec2(cell_pos.x + cell_size, cell_pos.y + cell_size),
                    color, 2.0f);
            }
        }

        // Draw grid lines
        if (m_show_grid_lines)
        {
            for (int row = 0; row <= m_shared_grid_state.rows; ++row)
            {
                ImVec2 start(grid_offset.x, grid_offset.y + row * cell_size);
                ImVec2 end(grid_offset.x + preview_width, grid_offset.y + row * cell_size);
                draw_list->AddLine(start, end, ImColor(60, 65, 75, 150), 1.0f);
            }
            for (int col = 0; col <= m_shared_grid_state.cols; ++col)
            {
                ImVec2 start(grid_offset.x + col * cell_size, grid_offset.y);
                ImVec2 end(grid_offset.x + col * cell_size, grid_offset.y + preview_height);
                draw_list->AddLine(start, end, ImColor(60, 65, 75, 150), 1.0f);
            }
        }

        // Handle mouse interaction
        handle_shared_grid_mouse_interaction(grid_offset, cell_size);

        ImGui::Dummy(ImVec2(preview_size, preview_size));
    }

    void AlgorithmComparisonScene::handle_shared_grid_mouse_interaction(
        const ImVec2& grid_offset,
        float cell_size)
    {
        ImVec2 mouse_pos = ImGui::GetMousePos();

        if (mouse_pos.x >= grid_offset.x &&
            mouse_pos.x <= grid_offset.x + cell_size * m_shared_grid_state.cols &&
            mouse_pos.y >= grid_offset.y &&
            mouse_pos.y <= grid_offset.y + cell_size * m_shared_grid_state.rows)
        {
            int col = static_cast<int>((mouse_pos.x - grid_offset.x) / cell_size);
            int row = static_cast<int>((mouse_pos.y - grid_offset.y) / cell_size);

            if (row >= 0 && row < m_shared_grid_state.rows &&
                col >= 0 && col < m_shared_grid_state.cols)
            {
                m_hovered_cell = {row, col};

                bool is_left_click = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
                bool is_right_click = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
                bool is_dragging = ImGui::IsMouseDown(ImGuiMouseButton_Left);

                if (is_right_click)
                {
                    // Erase on right click
                    if (m_shared_grid_state.grid[row][col] != algorithms::GridCellType::START &&
                        m_shared_grid_state.grid[row][col] != algorithms::GridCellType::TARGET)
                    {
                        m_shared_grid_state.grid[row][col] = algorithms::GridCellType::EMPTY;
                        sync_shared_grid_to_algorithms();
                    }
                }
                else if (is_left_click || (m_is_dragging && is_dragging))
                {
                    m_is_dragging = is_dragging;

                    switch (m_current_grid_tool)
                    {
                        case algorithms::GridToolMode::DRAW_WALLS:
                            if (m_shared_grid_state.grid[row][col] != algorithms::GridCellType::START &&
                                m_shared_grid_state.grid[row][col] != algorithms::GridCellType::TARGET)
                            {
                                m_shared_grid_state.grid[row][col] = algorithms::GridCellType::WALL;
                                sync_shared_grid_to_algorithms();
                            }
                            break;

                        case algorithms::GridToolMode::DRAW_WEIGHTS:
                            if (m_shared_grid_state.grid[row][col] != algorithms::GridCellType::START &&
                                m_shared_grid_state.grid[row][col] != algorithms::GridCellType::TARGET &&
                                m_shared_grid_state.grid[row][col] != algorithms::GridCellType::WALL)
                            {
                                int current_weight = 1;
                                if (m_shared_grid_state.grid[row][col] >= algorithms::GridCellType::WEIGHT_1 &&
                                    m_shared_grid_state.grid[row][col] <= algorithms::GridCellType::WEIGHT_5)
                                {
                                    current_weight = static_cast<int>(m_shared_grid_state.grid[row][col]) -
                                                    static_cast<int>(algorithms::GridCellType::WEIGHT_1) + 1;
                                }
                                int new_weight = (current_weight % 5) + 1;
                                m_shared_grid_state.set_weight(row, col, new_weight);
                                sync_shared_grid_to_algorithms();
                            }
                            break;

                        case algorithms::GridToolMode::PLACE_START:
                            m_shared_grid_state.set_start(row, col);
                            sync_shared_grid_to_algorithms();
                            break;

                        case algorithms::GridToolMode::PLACE_TARGET:
                            m_shared_grid_state.set_target(row, col);
                            sync_shared_grid_to_algorithms();
                            break;

                        case algorithms::GridToolMode::ERASE:
                            if (m_shared_grid_state.grid[row][col] != algorithms::GridCellType::START &&
                                m_shared_grid_state.grid[row][col] != algorithms::GridCellType::TARGET)
                            {
                                m_shared_grid_state.grid[row][col] = algorithms::GridCellType::EMPTY;
                                sync_shared_grid_to_algorithms();
                            }
                            break;

                        default:
                            break;
                    }
                }
                else
                {
                    m_is_dragging = false;
                }
            }
        }
        else
        {
            m_hovered_cell = {-1, -1};
        }
    }

    void AlgorithmComparisonScene::render_shared_grid_info()
    {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Grid Info");

        ImGui::Text("Size: %d x %d", m_shared_grid_state.rows, m_shared_grid_state.cols);
        ImGui::Text("Walls: %d", m_shared_grid_state.get_wall_count());
        ImGui::Text("Weighted Cells: %d", m_shared_grid_state.get_weighted_count());
        ImGui::Text("Start: (%d, %d)", m_shared_grid_state.start.row, m_shared_grid_state.start.col);
        ImGui::Text("Target: (%d, %d)", m_shared_grid_state.target.row, m_shared_grid_state.target.col);
    }

    void AlgorithmComparisonScene::render_shared_grid_legend()
    {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Legend");

        auto render_item = [](const char* label, ImColor color)
        {
            ImGui::ColorButton(("##" + std::string(label)).c_str(), color,
                ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop, ImVec2(16, 16));
            ImGui::SameLine();
            ImGui::TextUnformatted(label);
        };

        render_item("Empty", algorithms::COLOR_EMPTY);
        render_item("Wall", algorithms::COLOR_WALL);
        render_item("Start", algorithms::COLOR_START);
        render_item("Target", algorithms::COLOR_TARGET);
        render_item("Weight 1", algorithms::COLOR_WEIGHT_1);
        render_item("Weight 2", algorithms::COLOR_WEIGHT_2);
        render_item("Weight 3", algorithms::COLOR_WEIGHT_3);
        render_item("Weight 4", algorithms::COLOR_WEIGHT_4);
        render_item("Weight 5", algorithms::COLOR_WEIGHT_5);
    }

    void AlgorithmComparisonScene::sync_shared_grid_to_algorithms()
    {
        const auto& left = m_comparison_manager->get_left_algorithm();
        const auto& right = m_comparison_manager->get_right_algorithm();

        // Convert shared grid to the format expected by algorithms
        std::vector<std::vector<algorithms::GridCellType>> grid_cells(
            m_shared_grid_state.rows,
            std::vector<algorithms::GridCellType>(m_shared_grid_state.cols));

        for (int row = 0; row < m_shared_grid_state.rows; ++row)
            for (int col = 0; col < m_shared_grid_state.cols; ++col)
                grid_cells[row][col] = m_shared_grid_state.grid[row][col];

        // Sync to left algorithm (which will automatically update its visualizer)
        if (left.is_valid())
        {
            if (auto* grid_algo = dynamic_cast<algorithms::GridPathfindingBase*>(left.algorithm.get()))
            {
                grid_algo->set_grid_cells(m_shared_grid_state.rows, m_shared_grid_state.cols, grid_cells);
                grid_algo->set_start(m_shared_grid_state.start.row, m_shared_grid_state.start.col);
                grid_algo->set_target(m_shared_grid_state.target.row, m_shared_grid_state.target.col);

                std::vector<int> flat_data = m_shared_grid_state.flatten();
                grid_algo->initialize(flat_data);  // This regenerates steps with new grid
            }
        }

        // Sync to right algorithm
        if (right.is_valid())
        {
            if (auto* grid_algo = dynamic_cast<algorithms::GridPathfindingBase*>(right.algorithm.get()))
            {
                grid_algo->set_grid_cells(m_shared_grid_state.rows, m_shared_grid_state.cols, grid_cells);
                grid_algo->set_start(m_shared_grid_state.start.row, m_shared_grid_state.start.col);
                grid_algo->set_target(m_shared_grid_state.target.row, m_shared_grid_state.target.col);

                std::vector<int> flat_data = m_shared_grid_state.flatten();
                grid_algo->initialize(flat_data);  // This regenerates steps with new grid
            }
        }

        m_visualizer.sync_shared_grid_to_visualizer(*m_visualizer.m_left->visualizer);
        m_visualizer.sync_shared_grid_to_visualizer(*m_visualizer.m_right->visualizer);
    }

    void AlgorithmComparisonScene::render_results_panel()
    {
        ImGui::SetNextWindowSize(
            DEFAULT_WINDOW_SIZE, 
            ImGuiCond_FirstUseEver
        );

        ImGui::Begin(
            "Parallel Comparison Results",
            &m_show_results_panel
        );

        const auto& left = m_comparison_manager->get_left_algorithm();
        const auto& right = m_comparison_manager->get_right_algorithm();
        auto result = m_comparison_manager->get_parallel_comparison_result();

        // Parallel execution stats
        ImGui::TextColored(
            ImVec4(0,1,1,1), 
            "Parallel Execution Efficiency: %.2f%%", 
            result.parallel_efficiency * 100.0
        );
        ImGui::Separator();

        // Create a table for side-by-side comparison
        if (ImGui::BeginTable("ResultsTable", 3, 
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            // Header
            ImGui::TableSetupColumn("Metric");
            ImGui::TableSetupColumn(left.name.c_str());
            ImGui::TableSetupColumn(right.name.c_str());
            ImGui::TableHeadersRow();

            // Steps
            // ImGui::TableNextRow();
            // ImGui::TableSetColumnIndex(0);
            // ImGui::Text("Total Steps");
            // ImGui::TableSetColumnIndex(1);
            // ImGui::Text("%zu", left.metrics.total_steps.load());
            // ImGui::TableSetColumnIndex(2);
            // ImGui::Text("%zu", right.metrics.total_steps.load());
            if (m_selected_visualization_type == algorithms::VisualizationType::PATH_FINDING_BASED_VISUALIZATION ||
                m_selected_visualization_type == algorithms::VisualizationType::GRAPH_BASED_VISUALIZATION)
            {
                // Total visited nodes
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Visited Nodes");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", left.metrics.visited_node_count.load());
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%zu", right.metrics.visited_node_count.load());

                // Total explored nodes
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Explored Nodes");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", left.metrics.explored_node_count.load());
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%zu", right.metrics.explored_node_count.load());
            }

            // Comparisons
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Comparisons");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", left.metrics.comparison_count.load());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%zu", right.metrics.comparison_count.load());

            if (m_selected_visualization_type == algorithms::VisualizationType::ARRAY_BASED_VISUALIZATION ||
                m_selected_visualization_type == algorithms::VisualizationType::COMPARISON_BASED_VISUALIZATION)
            {
                // Swaps
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Swaps");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%zu", left.metrics.swap_count.load());
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%zu", right.metrics.swap_count.load());
            }


            // Average step time
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Algorithm Time (us)");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%" PRId64, m_comparison_manager->get_left_algorithm()
                .metrics.execution_time.load());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%" PRId64, m_comparison_manager->get_right_algorithm()
                .metrics.execution_time.load());

            // Memory
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Peak Memory (bytes)");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu", left.metrics.peak_memory.load());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%zu", right.metrics.peak_memory.load());

            ImGui::EndTable();
        }

        // Winner announcement
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (result.has_winner())
        {
            const char* winner_name = result.winner == "left" ? 
                left.name.c_str() : right.name.c_str();
            
            ImGui::TextColored(ImVec4(0,1,0,1), "Winner: %s", winner_name);
            ImGui::Text("Performance Ratio: %.2fx", result.performance_ratio);
            ImGui::Text("Parallel Speedup: %.2fx", result.parallel_efficiency);
        }
        else if (m_comparison_manager->is_complete())
        {
            ImGui::TextColored(ImVec4(1,1,0,1), "Tie!");
        }

        ImGui::End();
    }

    void AlgorithmComparisonScene::render_performance_graphs()
    {
        ImGui::SetNextWindowSize(DEFAULT_WINDOW_SIZE, ImGuiCond_FirstUseEver);
        ImGui::Begin("Performance Graphs", &m_show_performance_graphs);

        const auto& left  = m_comparison_manager->get_left_algorithm();
        const auto& right = m_comparison_manager->get_right_algorithm();

        const ImVec4 COLOR_LEFT  = {0.2f, 0.6f, 1.0f, 1.0f};
        const ImVec4 COLOR_RIGHT = {1.0f, 0.4f, 0.2f, 1.0f};
        const ImVec2 PLOT_SIZE   = {-1.0f, 120.0f};  // full width, 120px tall

        // ── Comparisons over steps ──────────────────────────────────────────
        ImGui::SeparatorText("Cumulative Comparisons per Step");

        const auto& lc = left.comparisons_history;
        const auto& rc = right.comparisons_history;

        if (!lc.empty())
        {
            ImGui::TextColored(COLOR_LEFT, "%s", left.name.c_str());
            ImGui::SameLine();
        }
        if (!rc.empty())
        {
            ImGui::TextColored(COLOR_RIGHT, "%s", right.name.c_str());
        }

        float max_comparisons = 1.0f;
        if (!lc.empty()) max_comparisons = std::max(max_comparisons, lc.back());
        if (!rc.empty()) max_comparisons = std::max(max_comparisons, rc.back());

        if (!lc.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_PlotLines, COLOR_LEFT);
            ImGui::PlotLines(
                ("##lc_" + left.name).c_str(),
                lc.data(),
                static_cast<int>(lc.size()),
                0, nullptr, 0.0f, max_comparisons, PLOT_SIZE
            );
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::TextDisabled("No data yet — step forward to record");
        }

        if (!rc.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_PlotLines, COLOR_RIGHT);
            ImGui::PlotLines(
                ("##rc_" + right.name).c_str(),
                rc.data(),
                static_cast<int>(rc.size()),
                0, nullptr, 0.0f, max_comparisons, PLOT_SIZE
            );
            ImGui::PopStyleColor();
        }

        // ── Step time histogram ─────────────────────────────────────────────
        ImGui::Spacing();
        ImGui::SeparatorText("Step Time (ms)");

        const auto& lt = left.step_time_history_ms;
        const auto& rt = right.step_time_history_ms;

        float max_time = 0.01f;
        if (!lt.empty()) max_time = std::max(max_time, *std::max_element(lt.begin(), lt.end()));
        if (!rt.empty()) max_time = std::max(max_time, *std::max_element(rt.begin(), rt.end()));

        if (!lt.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, COLOR_LEFT);
            ImGui::PlotHistogram(
                ("##lt_" + left.name).c_str(),
                lt.data(),
                static_cast<int>(lt.size()),
                0, nullptr, 0.0f, max_time, PLOT_SIZE
            );
            ImGui::PopStyleColor();
        }

        if (!rt.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, COLOR_RIGHT);
            ImGui::PlotHistogram(
                ("##rt_" + right.name).c_str(),
                rt.data(),
                static_cast<int>(rt.size()),
                0, nullptr, 0.0f, max_time, PLOT_SIZE
            );
            ImGui::PopStyleColor();
        }

        ImGui::End();
    }

    void AlgorithmComparisonScene::sync_visualizers()
    {
        const auto& left = m_comparison_manager->get_left_algorithm();
        const auto& right = m_comparison_manager->get_right_algorithm();

        m_visualizer.initialize(left, right, &m_shared_grid_state);
    }

    void AlgorithmComparisonScene::render_graph_based_data_controls()
    {
        render_shared_graph_editor();
    }

    void AlgorithmComparisonScene::render_shared_graph_editor()
    {
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Shared Graph Editor");
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                           "Changes apply to BOTH algorithms simultaneously");
        ImGui::Separator();

        ImGui::Spacing();
        ImGui::Separator();

        // Layout selector
        ImGui::Text("Layout");

        const char* layout_names[] = {
            "Force Directed", "Circular", "Hierarchical", "Grid",
            "Radial Tree", "Concentric", "Spectral", "Spiral",
            "Bipartite", "Vertical Tree", "Horizontal Tree", "Random"
        };

        int current_layout = static_cast<int>(m_shared_graph_state.current_layout);
        if (ImGui::Combo("##Layout", &current_layout, layout_names, IM_ARRAYSIZE(layout_names)))
        {
            m_shared_graph_state.current_layout = static_cast<algorithms::GraphLayoutEngine::LayoutType>(current_layout);
            m_shared_graph_state.compute_layout(ImVec2(800, 600), m_shared_graph_state.current_layout);
            sync_shared_graph_to_algorithms();
        }


        // Node addition
        ImGui::Text("Add Node");
        static char node_label[64] = "";
        static int node_value = 0;

        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##NodeLabel", "Label", node_label, sizeof(node_label));
        ImGui::SetNextItemWidth(-1);
        ImGui::InputInt("##NodeValue", &node_value);

        if (ImGui::Button("Add Node", ImVec2(-1, 0)))
        {
            std::string label = node_label;
            if (label.empty()) label = "Node " + std::to_string(m_shared_graph_state.graph.next_node_id);
            m_shared_graph_state.add_node(label, node_value);
            sync_shared_graph_to_algorithms();
            node_label[0] = '\0';
            node_value = 0;
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Edge addition
        ImGui::Text("Add Edge");
        static int edge_from = 0;
        static int edge_to = 0;
        static int edge_weight = 1;

        ImGui::SetNextItemWidth(-1);
        ImGui::InputInt("##EdgeFrom", &edge_from);
        ImGui::SetNextItemWidth(-1);
        ImGui::InputInt("##EdgeTo", &edge_to);
        ImGui::SetNextItemWidth(-1);
        ImGui::InputInt("##EdgeWeight", &edge_weight);

        if (ImGui::Button("Add Edge", ImVec2(-1, 0)))
        {
            if (m_shared_graph_state.add_edge(static_cast<size_t>(edge_from),
                                               static_cast<size_t>(edge_to),
                                               edge_weight))
            {
                sync_shared_graph_to_algorithms();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Graph properties
        ImGui::Text("Graph Properties");

        bool is_directed = m_shared_graph_state.graph.is_directed;
        if (ImGui::Checkbox("Directed Graph", &is_directed))
        {
            m_shared_graph_state.set_directed(is_directed);
            sync_shared_graph_to_algorithms();
        }

        bool is_weighted = m_shared_graph_state.graph.is_weighted;
        if (ImGui::Checkbox("Weighted Graph", &is_weighted))
        {
            m_shared_graph_state.set_weighted(is_weighted);
            sync_shared_graph_to_algorithms();
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Start/Target nodes
        ImGui::Text("Start / Target Nodes");

        int start_node = static_cast<int>(m_shared_graph_state.graph.start_node);
        if (ImGui::InputInt("Start Node", &start_node))
        {
            if (start_node >= 0 && m_shared_graph_state.graph.has_node(static_cast<size_t>(start_node)))
            {
                m_shared_graph_state.set_start_node(static_cast<size_t>(start_node));
                sync_shared_graph_to_algorithms();
            }
        }

        int target_node = m_shared_graph_state.graph.target_node.has_value()
            ? static_cast<int>(m_shared_graph_state.graph.target_node.value()) : -1;
        if (ImGui::InputInt("Target Node", &target_node))
        {
            if (target_node >= 0 && m_shared_graph_state.graph.has_node(static_cast<size_t>(target_node)))
            {
                m_shared_graph_state.set_target_node(static_cast<size_t>(target_node));
                sync_shared_graph_to_algorithms();
            }
            else
            {
                m_shared_graph_state.graph.target_node.reset();
                sync_shared_graph_to_algorithms();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Presets
        ImGui::Text("Presets");

        if (ImGui::Button("Path (5 nodes)", ImVec2(-1, 0)))
        {
            m_shared_graph_state.create_path_graph(5);
            sync_shared_graph_to_algorithms();
        }

        if (ImGui::Button("Cycle (6 nodes)", ImVec2(-1, 0)))
        {
            m_shared_graph_state.create_cycle_graph(6);
            sync_shared_graph_to_algorithms();
        }

        if (ImGui::Button("Complete (5 nodes)", ImVec2(-1, 0)))
        {
            m_shared_graph_state.create_complete_graph(5);
            sync_shared_graph_to_algorithms();
        }

        if (ImGui::Button("Star (6 nodes)", ImVec2(-1, 0)))
        {
            m_shared_graph_state.create_star_graph(6);
            sync_shared_graph_to_algorithms();
        }

        if (ImGui::Button("Binary Tree (15 nodes)", ImVec2(-1, 0)))
        {
            m_shared_graph_state.create_binary_tree(15);
            sync_shared_graph_to_algorithms();
        }

        if (ImGui::Button("Clear Graph", ImVec2(-1, 0)))
        {
            m_shared_graph_state.clear();
            sync_shared_graph_to_algorithms();
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Display options
        ImGui::Text("Display");
        ImGui::Checkbox("Show Edge Weights", &m_show_edge_weights);
        ImGui::Checkbox("Show Node Labels", &m_show_node_labels);
        ImGui::Checkbox("Show Node IDs", &m_show_node_ids);
        ImGui::Checkbox("Show Distances", &m_show_distances);

        ImGui::Spacing();

        // Graph info
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Graph Info");
        ImGui::Text("Nodes: %zu", m_shared_graph_state.graph.node_count());
        ImGui::Text("Edges: %zu", m_shared_graph_state.graph.edge_count());

        if (m_shared_graph_state.graph.node_count() > 1)
        {
            float density = 2.0f * m_shared_graph_state.graph.edge_count() /
                           (m_shared_graph_state.graph.node_count() *
                            (m_shared_graph_state.graph.node_count() - 1));
            ImGui::Text("Density: %.3f", density);
        }
    }

    void AlgorithmComparisonScene::sync_shared_graph_to_algorithms()
    {
        const auto& left = m_comparison_manager->get_left_algorithm();
        const auto& right = m_comparison_manager->get_right_algorithm();

        // Sync to left algorithm
        if (left.is_valid() && left.visualizer)
        {
            if (auto* graph_viz = dynamic_cast<algorithms::GraphBasedVisualizer*>(left.visualizer.get()))
            {
                graph_viz->sync_from_shared_state(m_shared_graph_state);
                graph_viz->set_graph_layout(get_layout_name(m_shared_graph_state.current_layout));
            }
        }

        // Sync to right algorithm
        if (right.is_valid() && right.visualizer)
        {
            if (auto* graph_viz = dynamic_cast<algorithms::GraphBasedVisualizer*>(right.visualizer.get()))
            {
                graph_viz->sync_from_shared_state(m_shared_graph_state);
                graph_viz->set_graph_layout(get_layout_name(m_shared_graph_state.current_layout));
            }
        }

        // Also sync display options
        sync_graph_display_options();
    }

    std::string AlgorithmComparisonScene::get_layout_name(algorithms::GraphLayoutEngine::LayoutType type)
    {
        switch (type)
        {
            case algorithms::GraphLayoutEngine::LayoutType::FORCE_DIRECTED: return "Force Directed";
            case algorithms::GraphLayoutEngine::LayoutType::CIRCULAR: return "Circular";
            case algorithms::GraphLayoutEngine::LayoutType::HIERARCHICAL: return "Hierarchical";
            case algorithms::GraphLayoutEngine::LayoutType::GRID: return "Grid";
            case algorithms::GraphLayoutEngine::LayoutType::RADIAL_TREE: return "Radial Tree";
            case algorithms::GraphLayoutEngine::LayoutType::CONCENTRIC: return "Concentric";
            case algorithms::GraphLayoutEngine::LayoutType::SPECTRAL: return "Spectral";
            case algorithms::GraphLayoutEngine::LayoutType::SPIRAL: return "Spiral";
            case algorithms::GraphLayoutEngine::LayoutType::BIPARTITE: return "Bipartite";
            case algorithms::GraphLayoutEngine::LayoutType::VERTICAL_TREE: return "Vertical Tree";
            case algorithms::GraphLayoutEngine::LayoutType::HORIZONTAL_TREE: return "Horizontal Tree";
            case algorithms::GraphLayoutEngine::LayoutType::RANDOM: return "Random";
            default: return "Hierarchical";
        }
    }

    void AlgorithmComparisonScene::sync_graph_display_options()
    {
        const auto& left = m_comparison_manager->get_left_algorithm();
        const auto& right = m_comparison_manager->get_right_algorithm();

        auto sync_options = [&](algorithms::IAlgorithmVisualizer* viz) {
            if (auto* graph_viz = dynamic_cast<algorithms::GraphBasedVisualizer*>(viz))
            {
                graph_viz->set_show_edge_weights(m_show_edge_weights);
                graph_viz->set_show_node_labels(m_show_node_labels);
                graph_viz->set_show_node_ids(m_show_node_ids);
                graph_viz->set_show_distances(m_show_distances);
            }
        };

        if (left.visualizer) sync_options(left.visualizer.get());
        if (right.visualizer) sync_options(right.visualizer.get());
    }


    void AlgorithmComparisonScene::setup_main_menu()
    {
        std::vector<ui::components::MainMenu::MenuItem> view_items = {
            {"Selection Panel", [this]() {
                m_show_selection_panel = !m_show_selection_panel;
            }, nullptr, "F1"},
            {"Comparison View", [this]() {
                m_show_comparison_view = !m_show_comparison_view;
            }, nullptr, "F2"},
            {"Controls", [this]() {
                m_show_control_panel = !m_show_control_panel;
            }, nullptr, "F3"},
            {"Data Controls", [this]() {
                m_show_data_control = !m_show_data_control;
            }, nullptr, "F4"},
            {"Results", [this]() {
                m_show_results_panel = !m_show_results_panel;
            }, nullptr, "F5"},
            {"Performance Graphs", [this]() {
                m_show_performance_graphs = !m_show_performance_graphs;
            }, nullptr, "F6"},
        };

        m_main_menu->add_menu("View", std::move(view_items));
    }

    void AlgorithmComparisonScene::rebuild_category_cache()
    {
        m_cached_category_names.clear();
        m_cached_category_names.reserve(m_cached_categorized_algorithms.size());

        for (const auto& [name, _] : m_cached_categorized_algorithms)
        {
            m_cached_category_names.push_back(name);
        }

        if (!m_cached_category_names.empty() && m_selected_category.empty())
        {
            m_selected_category = m_cached_category_names[1];
            m_current_display_algorithms = m_cached_categorized_algorithms[m_selected_category];
        }

        m_selected_visualization_type = m_current_display_algorithms[0]->visualization;

        m_cache_dirty = false;
    }

} // namespace c2l::scenes