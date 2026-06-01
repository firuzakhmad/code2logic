#include "algorithms/visualizers/path_finding_visualizer.hpp"
#include "algorithms/grid_dijkstra.hpp"
#include "algorithms/jump_point_search.hpp"
#include "algorithms/best_first_search.hpp"
#include "algorithms/grid_bfs.hpp"
#include "algorithms/grid_dfs.hpp"
#include "algorithms/theta_star.hpp"
#include "algorithms/core/heuristic_type.hpp"


#include "core/utils/logger/logger.hpp"
#include <imgui_internal.h>
#include <random>
#include <algorithm>
#include <cmath>




namespace c2l::algorithms
{
    PathFindingVisualizer::PathFindingVisualizer(
        ui::managers::IconManager& icon_manager,
        const VisualizationConfig& config)
        : m_icon_manager{icon_manager}
        , m_config{config}
    {
        set_grid_size(20, 20);
        set_start(5, 5);
        set_target(14, 14);
        LOG_DEBUG("PathFindingVisualizer created");
    }

    void PathFindingVisualizer::initialize(
        ISimpleAlgorithm* execution,
        const IAlgorithmMetadata* metadata,
        const bool show_sidebar_controller)
    {
        m_show_sidebar_controller = show_sidebar_controller;

        // Removing observer from previous algorithm
        if (m_execution)
        {
            m_execution->remove_observer(this);
        }

        m_execution = execution;
        m_metadata = metadata;

        // Add observer to track step changes
        if (m_execution)
        {
            m_execution->add_observer(this);
        }

        reset_visualization();
        sync_algorithm_with_grid();

        LOG_DEBUG(
            "PathFindingVisualizer initialized for {}",
            m_metadata ? m_metadata->get_display_name() : "Unknown"
        );
    }

    void PathFindingVisualizer::on_step_changed()
    {
        // Algorithm step changed - update visualization immediately
        if (m_execution)
        {
            update_algorithm_state();
        }
    }

    void PathFindingVisualizer::update(double delta_time)
    {
        // Update any visual animations
        static float animation_time = 0.0f;
        animation_time += static_cast<float>(delta_time);
        if (animation_time > 0.016f) // ~60fps
        {
            animation_time = 0.0f;
            // Update any visual effects if needed
        }
    }

    void PathFindingVisualizer::render()
    {
        ImGui::BeginChild(
            "PathFindingMain", 
            ImVec2(0, 0), 
            false
        );

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float sidebar_width = std::clamp(avail.x * 0.32f, 320.0f, 450.0f);
        if (!m_show_sidebar_controller)
            sidebar_width = 0;
        // Grid View
        ImGui::BeginChild(
            "GridView", 
            ImVec2(avail.x - sidebar_width, avail.y), 
            true
        );
        render_grid();
        handle_mouse_interaction();
        ImGui::EndChild();

        // Sidebar - Only for grid editing, no playback controls
        if (m_show_sidebar_controller)
        {
            ImGui::SameLine();

            ImGui::BeginChild(
                "Sidebar",
                ImVec2(sidebar_width, avail.y),
                true
            );


            render_grid_editor_panel();
            ImGui::Separator();
            render_info_panel();
            ImGui::Separator();
            render_legend_panel();

            ImGui::EndChild();
        }

        ImGui::EndChild();
    }

    VisualizationType PathFindingVisualizer::get_visualization_type() const
    {
        return VisualizationType::PATH_FINDING_BASED_VISUALIZATION;
    }

    bool PathFindingVisualizer::supports_algorithm(const AlgorithmType& type) const
    {
        return type == AlgorithmType::BFS || 
               type == AlgorithmType::DFS ||
               type == AlgorithmType::DIJKSTRA || 
               type == AlgorithmType::A_STAR ||
               type == AlgorithmType::GRID_A_STAR || 
               type == AlgorithmType::JUMP_POINT_SEARCH ||
               type == AlgorithmType::THETA_STAR || 
               type == AlgorithmType::BEST_FIRST_SEARCH;
    }

    void PathFindingVisualizer::set_visualization_style(VisualizationStyle style)
    {
        // Apply style settings
    }

    void PathFindingVisualizer::render_grid()
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 viewport_pos = ImGui::GetCursorScreenPos();
        ImVec2 viewport_size = ImGui::GetContentRegionAvail();

        float cell_size = std::min(
            viewport_size.x / static_cast<float>(m_state.cols),
            viewport_size.y / static_cast<float>(m_state.rows)
        );
        float grid_width = cell_size * static_cast<float>(m_state.cols);
        float grid_height = cell_size * static_cast<float>(m_state.rows);

        // Center grid
        ImVec2 grid_offset(
            viewport_pos.x + (viewport_size.x - grid_width) * 0.5f,
            viewport_pos.y + (viewport_size.y - grid_height) * 0.5f
        );

        // Draw background
        draw_list->AddRectFilled(
            grid_offset,
            ImVec2(grid_offset.x + grid_width, grid_offset.y + grid_height),
            ImColor(25, 30, 40, 255)
        );

        // Draw cells
        for (int row = 0; row < m_state.rows; ++row)
        {
            for (int col = 0; col < m_state.cols; ++col)
            {
                ImVec2 cell_pos(
                    grid_offset.x + static_cast<float>(col) * cell_size,
                    grid_offset.y + static_cast<float>(row) * cell_size
                );

                bool is_current = (m_state.current_node.row == row && m_state.current_node.col == col);

                render_cell(row, col, cell_pos, cell_size);

                // Drawing current node pulse effect
                if (is_current)
                {
                    float pulse = (std::sin(ImGui::GetTime() * 10.0f) + 1.0f) * 0.5f;
                    draw_list->AddRect(cell_pos,
                        ImVec2(cell_pos.x + cell_size, cell_pos.y + cell_size),
                        ImColor(255, 255, 255, static_cast<int>(150 + pulse * 100)),
                        4.0f, 0, 3.0f);
                }
            }
        }

        // Drawing grid lines
        if (m_show_grid_lines)
        {
            for (int row = 0; row <= m_state.rows; ++row)
            {
                ImVec2 start(
                    grid_offset.x, 
                    grid_offset.y + static_cast<float>(row) * cell_size
                );
                ImVec2 end(
                    grid_offset.x + grid_width, 
                    grid_offset.y + static_cast<float>(row) * cell_size
                );
                draw_list->AddLine(
                    start, 
                    end, 
                    ImColor(60, 65, 75, 150), 
                    1.0f
                );
            }
            for (int col = 0; col <= m_state.cols; ++col)
            {
                ImVec2 start(
                    grid_offset.x + static_cast<float>(col) * cell_size, 
                    grid_offset.y
                );
                ImVec2 end(
                    grid_offset.x + static_cast<float>(col) * cell_size, 
                    grid_offset.y + grid_height
                );
                draw_list->AddLine(
                    start, 
                    end, 
                    ImColor(60, 65, 75, 150), 
                    1.0f
                );
            }
        }

        // Update hovered cell
        ImVec2 mouse_pos = ImGui::GetMousePos();
        if (mouse_pos.x >= grid_offset.x && mouse_pos.x <= grid_offset.x + grid_width &&
            mouse_pos.y >= grid_offset.y && mouse_pos.y <= grid_offset.y + grid_height)
        {
            int col = static_cast<int>((mouse_pos.x - grid_offset.x) / cell_size);
            int row = static_cast<int>((mouse_pos.y - grid_offset.y) / cell_size);

            if (row >= 0 && row < m_state.rows && col >= 0 && col < m_state.cols)
            {
                m_hovered_cell = {row, col};

                // Draw hover effect
                ImVec2 hover_pos(
                    grid_offset.x + static_cast<float>(col) * cell_size,
                    grid_offset.y + static_cast<float>(row) * cell_size
                );
                draw_list->AddRect(hover_pos,
                    ImVec2(hover_pos.x + cell_size, hover_pos.y + cell_size),
                    ImColor(255, 255, 255, 100), 
                    4.0f, 
                    0, 
                    2.0f
                );
            }
            else
            {
                m_hovered_cell = {-1, -1};
            }
        }
        else
        {
            m_hovered_cell = {-1, -1};
        }
    }

    void PathFindingVisualizer::render_cell(
        int row, 
        int col, 
        const ImVec2& pos, 
        float size)
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        const auto& cell = cell_at(row, col);

        // Determine color based on cell state
        ImColor color;

        // Priority: Current > Path > Visited/Frontier > Start/Target > Walls/Empty
        if (cell.on_path && (m_state.is_complete || m_state.path_found))
        {
            color = COLOR_PATH;
        }
        else if (m_state.current_node.row == row && 
                 m_state.current_node.col == col)
        {
            color = COLOR_CURRENT;
        }
        else if (cell.type == GridCellType::START)
        {
            color = COLOR_START;
        }
        else if (cell.type == GridCellType::TARGET)
        {
            color = COLOR_TARGET;
        }
        else if (cell.visited)
        {
            color = COLOR_VISITED;
        }
        else if (cell.in_frontier)
        {
            color = COLOR_FRONTIER;
        }
        else if (cell.type == GridCellType::WALL)
        {
            color = COLOR_WALL;
        }
        else if (cell.type >= GridCellType::WEIGHT_1 && 
                 cell.type <= GridCellType::WEIGHT_5)
        {
            int weight_val = static_cast<int>(cell.type) - static_cast<int>(GridCellType::WEIGHT_1) + 1;
            switch (weight_val)
            {
                case 1: color = COLOR_WEIGHT_1; break;
                case 2: color = COLOR_WEIGHT_2; break;
                case 3: color = COLOR_WEIGHT_3; break;
                case 4: color = COLOR_WEIGHT_4; break;
                default: color = COLOR_WEIGHT_5; break;
            }
        }
        else
        {
            color = COLOR_EMPTY;
        }

        // Draw cell background with rounded corners
        draw_list->AddRectFilled(
            pos, 
            ImVec2(pos.x + size, pos.y + size), 
            color, 
            4.0f
        );

        // Draw cell border
        draw_list->AddRect(
            pos, 
            ImVec2(pos.x + size, pos.y + size), 
            ImColor(255, 255, 255, 30), 
            4.0f
        );

        // Draw cell content
        if (m_show_weights && 
            cell.type >= GridCellType::WEIGHT_1 && 
            cell.type <= GridCellType::WEIGHT_5)
        {
            int weight = static_cast<int>(cell.type) - static_cast<int>(GridCellType::WEIGHT_1) + 1;
            std::string weight_str = std::to_string(weight);
            ImVec2 text_size = ImGui::CalcTextSize(weight_str.c_str());
            draw_list->AddText(
                ImVec2(pos.x + (size - text_size.x) * 0.5f, pos.y + (size - text_size.y) * 0.5f),
                ImColor(255, 255, 255, 200), 
                weight_str.c_str()
            );
        }

        if (m_show_coordinates && 
            cell.type == GridCellType::EMPTY && 
            !cell.visited && !cell.in_frontier)
        {
            std::string coord = std::to_string(row) + "," + std::to_string(col);
            ImVec2 text_size = ImGui::CalcTextSize(coord.c_str());
            if (text_size.x < size && text_size.y < size)
            {
                draw_list->AddText(
                    ImVec2(
                        pos.x + (size - text_size.x) * 0.5f, 
                        pos.y + (size - text_size.y) * 0.5f
                    ),
                    ImColor(100, 100, 120, 150), 
                    coord.c_str()
                );
            }
        }

        // Draw distance/g-score if available
        if (cell.distance >= 0 && (cell.visited || cell.in_frontier))
        {
            std::string dist_str = std::to_string(cell.distance);
            ImVec2 text_size = ImGui::CalcTextSize(dist_str.c_str());
            if (text_size.x < size * 0.6f)
            {
                draw_list->AddText(
                    ImVec2(
                        pos.x + size - text_size.x - 3, 
                        pos.y + size - text_size.y - 3
                    ),
                    ImColor(255, 255, 255, 180), 
                    dist_str.c_str()
                );
            }
        }
    }

    void PathFindingVisualizer::render_grid_editor_panel()
    {
        ImGui::TextColored(ImColor(100, 200, 255), "Grid Editor");
        ImGui::Dummy({0, 10});

        // Tool selection
        ImGui::Text("Tools");
        int current_tool = static_cast<int>(m_current_tool);
        for (int i = 0; i < IM_ARRAYSIZE(TOOL_NAMES); ++i)
        {
            ImGui::PushID(i);
            if (ImGui::Selectable(TOOL_NAMES[i], current_tool == i))
            {
                m_current_tool = static_cast<GridToolMode>(i);
            }
            ImGui::PopID();
        }

        ImGui::Dummy({0, 10});
        ImGui::Separator();
        ImGui::Dummy({0, 10});

        // Algorithm info
        ImGui::Text("Algorithm:");
        ImGui::SameLine();
        if (m_metadata)
        {
            ImGui::TextColored(
                ImColor(100, 255, 100), 
                "%s", 
                m_metadata->get_display_name().c_str()
            );
        }
        else
        {
            ImGui::TextColored(
                ImColor(255, 100, 100), 
                "No algorithm loaded"
            );
        }

        ImGui::Separator();

        // Grid controls
        ImGui::Text("Grid Configuration");

        ImGui::SetNextItemWidth(-1);
        int grid_size[2] = {m_state.rows, m_state.cols};
        if (ImGui::InputInt2("##Size (Rows, Cols)", grid_size))
        {
            grid_size[0] = std::clamp(grid_size[0], 5, 50);
            grid_size[1] = std::clamp(grid_size[1], 5, 50);
            set_grid_size(grid_size[0], grid_size[1]);
        }

        if (ImGui::Button("Clear Grid", ImVec2(-1, 0)))
        {
            clear_grid();
        }

        if (ImGui::Button("Random Maze", ImVec2(-1, 0)))
        {
            generate_random_maze(0.35f);
        }

        if (ImGui::Button("Recursive Maze", ImVec2(-1, 0)))
        {
            generate_recursive_backtracking_maze();
        }

        if (ImGui::Button("Sync with Algorithm", ImVec2(-1, 0)))
        {
            sync_algorithm_with_grid();
        }

        ImGui::Separator();

        // Display options
        ImGui::Text("Display");
        ImGui::Checkbox("Grid Lines", &m_show_grid_lines);
        ImGui::Checkbox("Show Weights", &m_show_weights);
        ImGui::Checkbox("Show Coordinates", &m_show_coordinates);
    }

    void PathFindingVisualizer::render_info_panel()
    {
        ImGui::TextColored(ImColor(100, 200, 255), "Statistics");

        ImGui::Text(
            "Status: %s",
            m_state.is_complete ? (m_state.path_found ? "Path Found!" : "No Path") : "Searching..."
        );

        if (m_state.path_found && !m_state.path.empty())
        {
            ImGui::Text("Path Length: %zu steps", m_state.path.size());
            ImGui::Text("Path Cost: %zu", m_state.path_cost);
        }

        ImGui::Text("Nodes Visited: %zu", m_state.visited_order.size());
        ImGui::Text("Nodes Explored: %zu", m_state.frontier_order.size());

        if (m_execution)
        {
            ImGui::Text("Step: %zu / %zu",
                m_execution->get_current_step_index() + 1,
                m_execution->get_step_count());
        }

        if (m_state.current_node.row >= 0)
        {
            ImGui::Text(
                "Current: (%d, %d)", 
                m_state.current_node.row, 
                m_state.current_node.col
            );
        }
    }

    void PathFindingVisualizer::render_legend_panel()
    {
        auto render_legend_item = [](const char* label, ImColor color)
        {
            ImGui::ColorButton(
                ("##" + std::string(label)).c_str(),
                color,
                ImGuiColorEditFlags_NoTooltip |
                ImGuiColorEditFlags_NoDragDrop,
                ImVec2(18, 18)
            );

            ImGui::SameLine();
            ImGui::TextUnformatted(label);
        };

        // =========================================================
        // LEFT GROUP : MAIN LEGEND
        // =========================================================
        ImGui::BeginGroup();

        ImGui::TextColored(ImColor(100, 200, 255), "Legend");
        ImGui::Separator();

        render_legend_item("Empty",    COLOR_EMPTY);
        render_legend_item("Wall",     COLOR_WALL);
        render_legend_item("Start",    COLOR_START);
        render_legend_item("Target",   COLOR_TARGET);
        render_legend_item("Visited",  COLOR_VISITED);
        render_legend_item("Frontier", COLOR_FRONTIER);
        render_legend_item("Current",  COLOR_CURRENT);
        render_legend_item("Path",     COLOR_PATH);

        ImGui::EndGroup();

        // =========================================================
        // RIGHT GROUP : WEIGHTS
        // =========================================================
        ImGui::SameLine(0.0f, 40.0f);

        ImGui::BeginGroup();

        ImGui::TextColored(ImColor(255, 200, 100), "Weighted Cells");
        ImGui::Separator();

        for (int w = 1; w <= 5; ++w)
        {
            ImColor weight_color;

            switch (w)
            {
                case 1: weight_color = COLOR_WEIGHT_1; break;
                case 2: weight_color = COLOR_WEIGHT_2; break;
                case 3: weight_color = COLOR_WEIGHT_3; break;
                case 4: weight_color = COLOR_WEIGHT_4; break;
                default: weight_color = COLOR_WEIGHT_5; break;
            }

            render_legend_item(
                ("Weight " + std::to_string(w)).c_str(),
                weight_color
            );
        }

        ImGui::EndGroup();
    }

    void PathFindingVisualizer::handle_mouse_interaction()
    {
        if (m_hovered_cell.first < 0) return;

        auto [row, col] = m_hovered_cell;
        bool is_left_click = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        bool is_right_click = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
        bool is_dragging = ImGui::IsMouseDown(ImGuiMouseButton_Left);

        // Quick right-click erase
        if (is_right_click)
        {
            if (cell_at(row, col).type != GridCellType::START &&
                cell_at(row, col).type != GridCellType::TARGET)
            {
                cell_at(row, col).type = GridCellType::EMPTY;
                cell_at(row, col).weight = 1;
                sync_algorithm_with_grid();
            }
            return;
        }

        // Tool-based left click/drag
        if (is_left_click || (m_is_dragging && is_dragging))
        {
            m_is_dragging = is_dragging;

            switch (m_current_tool)
            {
                case GridToolMode::DRAW_WALLS:
                    if (cell_at(row, col).type != GridCellType::START &&
                        cell_at(row, col).type != GridCellType::TARGET)
                    {
                        cell_at(row, col).type = GridCellType::WALL;
                        sync_algorithm_with_grid();
                    }
                    break;

                case GridToolMode::DRAW_WEIGHTS:
                    if (cell_at(row, col).type != GridCellType::START &&
                        cell_at(row, col).type != GridCellType::TARGET &&
                        cell_at(row, col).type != GridCellType::WALL)
                    {
                        int current_weight = cell_at(row, col).weight;
                        int new_weight = (current_weight % 5) + 1;
                        cell_at(row, col).weight = new_weight;
                        cell_at(row, col).type = static_cast<GridCellType>(
                            static_cast<int>(GridCellType::WEIGHT_1) + new_weight - 1);
                        sync_algorithm_with_grid();
                    }
                    break;

                case GridToolMode::PLACE_START:
                    if (m_state.start.row >= 0)
                    {
                        cell_at(m_state.start.row, m_state.start.col).type = GridCellType::EMPTY;
                        cell_at(m_state.start.row, m_state.start.col).weight = 1;
                    }
                    set_start(row, col);
                    sync_algorithm_with_grid();
                    break;

                case GridToolMode::PLACE_TARGET:
                    if (m_state.target.row >= 0)
                    {
                        cell_at(m_state.target.row, m_state.target.col).type = GridCellType::EMPTY;
                        cell_at(m_state.target.row, m_state.target.col).weight = 1;
                    }
                    set_target(row, col);
                    sync_algorithm_with_grid();
                    break;

                case GridToolMode::ERASE:
                    if (cell_at(row, col).type != GridCellType::START &&
                        cell_at(row, col).type != GridCellType::TARGET)
                    {
                        cell_at(row, col).type = GridCellType::EMPTY;
                        cell_at(row, col).weight = 1;
                        sync_algorithm_with_grid();
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

    void PathFindingVisualizer::set_grid_size(int rows, int cols)
    {
        m_state.rows = rows;
        m_state.cols = cols;
        m_state.grid.assign(
            static_cast<size_t>(rows), 
            std::vector<GridCellData>(static_cast<size_t>(cols))
        );

        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                cell_at(row, col).value = row * cols + col;
            }
        }

        set_start(std::min(rows - 2, 5), std::min(cols - 2, 5));
        set_target(std::min(rows - 2, rows - 5), std::min(cols - 2, cols - 5));

        if (m_execution)
        {
            sync_algorithm_with_grid();
        }
    }

    void PathFindingVisualizer::clear_grid()
    {
        for (int row = 0; row < m_state.rows; ++row)
        {
            for (int col = 0; col < m_state.cols; ++col)
            {
                cell_at(row, col).type = GridCellType::EMPTY;
                cell_at(row, col).weight = 1;
                cell_at(row, col).value = row * m_state.cols + col;
            }
        }

        set_start(std::min(m_state.start.row, m_state.rows - 2),
                  std::min(m_state.start.col, m_state.cols - 2));
        set_target(std::min(m_state.target.row, m_state.rows - 2),
                   std::min(m_state.target.col, m_state.cols - 2));

        if (m_execution)
        {
            sync_algorithm_with_grid();
        }
        reset_visualization();
    }

    void PathFindingVisualizer::generate_random_maze(float wall_density)
    {
        clear_grid();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);

        for (int row = 0; row < m_state.rows; ++row)
        {
            for (int col = 0; col < m_state.cols; ++col)
            {
                if ((row == m_state.start.row && col == m_state.start.col) ||
                    (row == m_state.target.row && col == m_state.target.col))
                {
                    continue;
                }

                if (dis(gen) < wall_density)
                {
                    cell_at(row, col).type = GridCellType::WALL;
                }
            }
        }

        if (m_execution)
        {
            sync_algorithm_with_grid();
        }
        reset_visualization();
    }

    void PathFindingVisualizer::generate_recursive_backtracking_maze()
    {
        clear_grid();

        for (int row = 0; row < m_state.rows; ++row)
        {
            for (int col = 0; col < m_state.cols; ++col)
            {
                if ((row != m_state.start.row || col != m_state.start.col) &&
                    (row != m_state.target.row || col != m_state.target.col))
                {
                    cell_at(row, col).type = GridCellType::WALL;
                }
            }
        }

        std::random_device rd;
        std::mt19937 gen(rd());

        std::function<void(int, int)> carve = [&](int row, int col)
        {
            std::vector<std::pair<int, int>> dirs = {{-2, 0}, {0, 2}, {2, 0}, {0, -2}};
            std::shuffle(dirs.begin(), dirs.end(), gen);

            for (auto [dr, dc] : dirs)
            {
                int new_row = row + dr;
                int new_col = col + dc;

                if (new_row > 0 && new_row < m_state.rows - 1 &&
                    new_col > 0 && new_col < m_state.cols - 1 &&
                    m_state.grid[static_cast<size_t>(new_row)][static_cast<size_t>(new_col)].type == GridCellType::WALL)
                {
                    m_state.grid[static_cast<size_t>(new_row)][static_cast<size_t>(new_col)].type = GridCellType::EMPTY;
                    m_state.grid[static_cast<size_t>(row + dr/2)][static_cast<size_t>(col + dc/2)].type = GridCellType::EMPTY;
                    carve(new_row, new_col);
                }
            }
        };

        carve(1, 1);

        if (m_execution)
        {
            sync_algorithm_with_grid();
        }
        reset_visualization();
    }

    void PathFindingVisualizer::set_start(int row, int col)
    {
        if (m_state.start.row >= 0 && m_state.start.row < m_state.rows &&
            m_state.start.col >= 0 && m_state.start.col < m_state.cols)
        {
            cell_at(m_state.start.row, m_state.start.col).type = GridCellType::EMPTY;
        }

        m_state.start = GridPosition{row, col};
        cell_at(row, col).type = GridCellType::START;
        cell_at(row, col).weight = 1;
    }

    void PathFindingVisualizer::set_target(int row, int col)
    {
        if (m_state.target.row >= 0 && 
            m_state.target.row < m_state.rows &&
            m_state.target.col >= 0 && 
            m_state.target.col < m_state.cols)
        {
            cell_at(m_state.target.row, m_state.target.col).type = GridCellType::EMPTY;
        }

        m_state.target = {row, col};
        cell_at(row, col).type = GridCellType::TARGET;
        cell_at(row, col).weight = 1;
    }

    void PathFindingVisualizer::reset_visualization()
    {
        m_state.visited_order.clear();
        m_state.frontier_order.clear();
        m_state.path.clear();
        m_state.current_node = GridPosition{-1, -1};
        m_state.comparisons = 0;
        m_state.path_cost = 0;
        m_state.is_complete = false;
        m_state.path_found = false;

        for (int row = 0; row < m_state.rows; ++row)
        {
            for (int col = 0; col < m_state.cols; ++col)
            {
                cell_at(row, col).visited = false;
                cell_at(row, col).in_frontier = false;
                cell_at(row, col).on_path = false;
                cell_at(row, col).g_score = 0.0f;
                cell_at(row, col).f_score = 0.0f;
                cell_at(row, col).distance = -1;
                cell_at(row, col).parent = GridPosition{-1, -1};
                cell_at(row, col).highlight_intensity = 0.0f;
            }
        }
    }

    void PathFindingVisualizer::update_algorithm_state()
    {
        if (!m_execution) return;

        const auto& step = m_execution->get_current_step();

        // Reset state before applying step
        for (int row = 0; row < m_state.rows; ++row)
        {
            for (int col = 0; col < m_state.cols; ++col)
            {
                cell_at(row, col).visited = false;
                cell_at(row, col).in_frontier = false;
                cell_at(row, col).on_path = false;
            }
        }

        m_state.visited_order.clear();
        m_state.frontier_order.clear();
        m_state.comparisons = step.visualization.comparison_count;
        m_state.is_complete = m_execution->is_complete();

        // Extracting visited nodes from visualization data
        for (size_t node_id : step.visualization.graph_state.visited_nodes)
        {
            int row = static_cast<int>(node_id / m_state.cols);
            int col = static_cast<int>(node_id % m_state.cols);
            if (row >= 0 && row < m_state.rows && col >= 0 && col < m_state.cols)
            {
                cell_at(row, col).visited = true;
                m_state.visited_order.emplace_back(row, col);
            }
        }

        // Extracting frontier nodes
        for (size_t node_id : step.visualization.graph_state.frontier_nodes)
        {
            int row = static_cast<int>(node_id / m_state.cols);
            int col = static_cast<int>(node_id % m_state.cols);
            if (row >= 0 && row < m_state.rows && col >= 0 && col < m_state.cols)
            {
                cell_at(row, col).in_frontier = true;
                m_state.frontier_order.emplace_back(row, col);
            }
        }

        // Extracting current node
        if (step.visualization.highlighted_index.has_value())
        {
            size_t node_id = step.visualization.highlighted_index.value();
            m_state.current_node = {
                static_cast<int>(node_id / m_state.cols),
                static_cast<int>(node_id % m_state.cols)
            };
        }
        else
        {
            m_state.current_node = GridPosition{-1, -1};
        }

        // Extracting path
        m_state.path.clear();
        for (size_t node_id : step.visualization.graph_state.path)
        {
            int row = static_cast<int>(node_id / m_state.cols);
            int col = static_cast<int>(node_id % m_state.cols);
            if (row >= 0 && row < m_state.rows && col >= 0 && col < m_state.cols)
            {
                cell_at(row, col).on_path = true;
                m_state.path.emplace_back(row, col);
            }
        }

        m_state.path_found = !m_state.path.empty();

        if (m_state.path_found)
        {
            m_state.path_cost = 0;
            for (const auto& pos : m_state.path)
            {
                if (pos.row >= 0 && pos.col >= 0)
                {
                    m_state.path_cost += static_cast<size_t>(cell_at(pos.row, pos.col).weight);
                }
            }
        }
    }

    void PathFindingVisualizer::sync_algorithm_with_grid()
    {
        if (!m_execution) return;

        // Prepare grid data
        size_t rows = static_cast<size_t>(m_state.rows);
        size_t cols = static_cast<size_t>(m_state.cols);

        std::vector<std::vector<GridCellType>> grid_cells(
            rows, std::vector<GridCellType>(cols));

        for (size_t row = 0; row < rows; ++row)
        {
            for (size_t col = 0; col < cols; ++col)
            {
                grid_cells[row][col] = static_cast<GridCellType>(
                    static_cast<int>(m_state.grid[row][col].type));
            }
        }

        if (auto* grid_astar = dynamic_cast<GridAStar*>(m_execution))
        {
            grid_astar->set_grid_cells(m_state.rows, m_state.cols, grid_cells);
            grid_astar->set_start(m_state.start.row, m_state.start.col);
            grid_astar->set_target(m_state.target.row, m_state.target.col);
            grid_astar->set_allow_diagonals(true);
            grid_astar->set_heuristic_type(HeuristicType::Octile);

            std::vector<int> flat_data;
            flat_data.reserve(rows * cols);
            for (size_t row = 0; row < rows; ++row)
            {
                for (size_t col = 0; col < cols; ++col)
                {
                    flat_data.push_back(m_state.grid[row][col].value);
                }
            }

            grid_astar->initialize(flat_data);
        }
        else if (auto* gps = dynamic_cast<JumpPointSearch*>(m_execution))
        {
            gps->set_grid_cells(m_state.rows, m_state.cols, grid_cells);
            gps->set_start(m_state.start.row, m_state.start.col);
            gps->set_target(m_state.target.row, m_state.target.col);
            gps->set_allow_diagonals(true);
            gps->set_heuristic_type(HeuristicType::Octile);

            std::vector<int> flat_data;
            flat_data.reserve(rows * cols);
            for (size_t row = 0; row < rows; ++row)
            {
                for (size_t col = 0; col < cols; ++col)
                {
                    flat_data.push_back(m_state.grid[row][col].value);
                }
            }

            gps->initialize(flat_data);
        }

        else if (auto* dijkstra = dynamic_cast<GridDijkstra*>(m_execution))
        {
            dijkstra->set_grid_cells(m_state.rows, m_state.cols, grid_cells);
            dijkstra->set_start(m_state.start.row, m_state.start.col);
            dijkstra->set_target(m_state.target.row, m_state.target.col);
            dijkstra->set_allow_diagonals(true);

            std::vector<int> flat_data;
            flat_data.reserve(rows * cols);
            for (size_t row = 0; row < rows; ++row)
                for (size_t col = 0; col < cols; ++col)
                    flat_data.push_back(m_state.grid[row][col].value);

            dijkstra->initialize(flat_data);
        }
        else if (auto* bfs = dynamic_cast<GridBFS*>(m_execution))
        {
            bfs->set_grid_cells(m_state.rows, m_state.cols, grid_cells);
            bfs->set_start(m_state.start.row, m_state.start.col);
            bfs->set_target(m_state.target.row, m_state.target.col);
            bfs->set_allow_diagonals(true);

            std::vector<int> flat_data;
            flat_data.reserve(rows * cols);
            for (size_t row = 0; row < rows; ++row)
                for (size_t col = 0; col < cols; ++col)
                    flat_data.push_back(m_state.grid[row][col].value);

            bfs->initialize(flat_data);
        }
        else if (auto* dfs = dynamic_cast<GridDFS*>(m_execution))
        {
            dfs->set_grid_cells(m_state.rows, m_state.cols, grid_cells);
            dfs->set_start(m_state.start.row, m_state.start.col);
            dfs->set_target(m_state.target.row, m_state.target.col);
            dfs->set_allow_diagonals(true);

            std::vector<int> flat_data;
            flat_data.reserve(rows * cols);
            for (size_t row = 0; row < rows; ++row)
                for (size_t col = 0; col < cols; ++col)
                    flat_data.push_back(m_state.grid[row][col].value);

            dfs->initialize(flat_data);
        }
        else if (auto* theta_star = dynamic_cast<ThetaStar*>(m_execution))
        {
            theta_star->set_grid_cells(m_state.rows, m_state.cols, grid_cells);
            theta_star->set_start(m_state.start.row, m_state.start.col);
            theta_star->set_target(m_state.target.row, m_state.target.col);
            theta_star->set_allow_diagonals(true);

            std::vector<int> flat_data;
            flat_data.reserve(rows * cols);
            for (size_t row = 0; row < rows; ++row)
                for (size_t col = 0; col < cols; ++col)
                    flat_data.push_back(m_state.grid[row][col].value);

            theta_star->initialize(flat_data);
        }
        else if (auto* best_first = dynamic_cast<BestFirstSearch*>(m_execution))
        {
            best_first->set_grid_cells(m_state.rows, m_state.cols, grid_cells);

            best_first->set_start(
                m_state.start.row,
                m_state.start.col
            );

            best_first->set_target(
                m_state.target.row,
                m_state.target.col
            );

            best_first->set_allow_diagonals(true);
            best_first->set_heuristic_type(HeuristicType::Octile);
            best_first->set_tie_breaking(true);

            std::vector<int> flat_data;
            flat_data.reserve(rows * cols);

            for (size_t row = 0; row < rows; ++row)
            {
                for (size_t col = 0; col < cols; ++col)
                {
                    flat_data.push_back(m_state.grid[row][col].value);
                }
            }

            best_first->initialize(flat_data);
        }
    }


    GridCellData& PathFindingVisualizer::cell_at(int row, int col)
    {
        return m_state.grid[static_cast<size_t>(row)][static_cast<size_t>(col)];
    }
    const GridCellData& PathFindingVisualizer::cell_at(int row, int col) const
    {
        return m_state.grid[static_cast<size_t>(row)][static_cast<size_t>(col)];
    }

    void PathFindingVisualizer::set_wall(int row, int col, bool is_wall)
    {
        if (row < 0 || row >= m_state.rows || col < 0 || col >= m_state.cols) return;
        if (cell_at(row, col).type == GridCellType::START ||
            cell_at(row, col).type == GridCellType::TARGET) return;

        cell_at(row, col).type = is_wall ? GridCellType::WALL : GridCellType::EMPTY;
    }

    void PathFindingVisualizer::set_weight(int row, int col, int weight)
    {
        if (row < 0 || row >= m_state.rows || col < 0 || col >= m_state.cols) return;
        if (cell_at(row, col).type == GridCellType::START ||
            cell_at(row, col).type == GridCellType::TARGET) return;

        weight = std::clamp(weight, 1, 5);
        cell_at(row, col).weight = weight;
        cell_at(row, col).type = static_cast<GridCellType>(
            static_cast<int>(GridCellType::WEIGHT_1) + weight - 1);
    }

    size_t PathFindingVisualizer::get_total_visited_nodes() const
    {
        return m_state.visited_order.size();
    }

    size_t PathFindingVisualizer::get_total_explored_nodes() const
    {
        return m_state.frontier_order.size();
    }


} // namespace c2l::algorithms