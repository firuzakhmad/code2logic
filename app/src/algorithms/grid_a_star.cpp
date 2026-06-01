#include "algorithms/grid_a_star.hpp"
#include "core/utils/logger/logger.hpp"
#include "core/utils/variables.hpp"

#include <algorithm>
#include <sstream>
#include <cmath>
#include <queue>


namespace c2l::algorithms
{
    GridAStar::GridAStar(core::JsonConfigManager& json_config_manager)
        : GridPathfindingBase(json_config_manager, AlgorithmType::GRID_A_STAR)
    {
        LOG_INFO(
            "Grid A* initialized on {}x{} grid. Start: ({},{}) Target: ({},{}) "
                "| time={}us steps={} mem={}B",
            m_rows,
            m_cols,
            m_start.row,
            m_start.col,
            m_target.row,
            m_target.col,
            m_algorithm_time_us,
            m_steps.size(),
            m_peak_memory_bytes
        );
    }

    float GridAStar::calculate_heuristic(
        const GridPosition& pos,
        const GridPosition& target
    ) const
    {
        const float dx = static_cast<float>(pos.col - target.col);
        const float dy = static_cast<float>(pos.row - target.row);

        switch (m_heuristic_type)
        {
            case HeuristicType::Euclidean:
                return std::sqrt(dx * dx + dy * dy);
            case HeuristicType::Manhattan:
                return std::abs(dx) + std::abs(dy);
            case HeuristicType::Chebyshev:
                return std::max(std::abs(dx), std::abs(dy));
            case HeuristicType::Octile:
                // Octagonal heuristic for 8-directional movement
                return std::max(
                        std::abs(dx),
                        std::abs(dy)) + (core::SQRT2 - 1.0f) * std::min(
                            std::abs(dx),
                            std::abs(dy));
            default:
                return std::sqrt(dx * dx + dy * dy);
        }
    }

    float GridAStar::get_movement_cost(
        const GridPosition& from, 
        const GridPosition& to
    ) const
    {
        // Check if diagonal movement
        bool is_diagonal = (from.row != to.row && from.col != to.col);
        
        // Base cost
        float cost = 1.0f;
        
        // Diagonal movement costs sqrt(2) ≈ 1.414
        if (is_diagonal && m_allow_diagonals)
        {
            cost = static_cast<float>(core::SQRT2);
        }
        
        // Apply terrain weight
        int weight = get_cell_weight(to);
        cost *= weight;
        
        return cost;
    }

    std::vector<GridPosition> GridAStar::get_neighbors(
        const GridPosition& pos
    ) const
    {
        std::vector<GridPosition> neighbors;
        
        // 4-directional movement
        const int dr4[] = {-1, 0, 1, 0};
        const int dc4[] = {0, 1, 0, -1};
        
        for (int i = 0; i < 4; ++i)
        {
            int nr = pos.row + dr4[i];
            int nc = pos.col + dc4[i];
            
            if (nr >= 0 && nr < m_rows && nc >= 0 && nc < m_cols)
            {
                // Skipping walls
                if (m_grid[nr][nc] != GridCellType::WALL)
                {
                    neighbors.emplace_back(nr, nc);
                }
            }
        }
        
        // 8-directional movement (diagonals)
        if (m_allow_diagonals)
        {
            const int dr8[] = {-1, -1, 1, 1};
            const int dc8[] = {-1, 1, -1, 1};
            
            for (int i = 0; i < 4; ++i)
            {
                int nr = pos.row + dr8[i];
                int nc = pos.col + dc8[i];
                
                if (nr >= 0 && nr < m_rows && nc >= 0 && nc < m_cols)
                {
                    // Skipping walls
                    if (m_grid[nr][nc] != GridCellType::WALL)
                    {
                        // Checking if diagonal movement is allowed (not cutting corners)
                        // This prevents moving diagonally through walls
                        bool can_move_diag = true;
                        if (m_grid[nr][pos.col] == GridCellType::WALL ||
                            m_grid[pos.row][nc] == GridCellType::WALL)
                        {
                            can_move_diag = false;
                        }
                        
                        if (can_move_diag)
                        {
                            neighbors.emplace_back(nr, nc);
                        }
                    }
                }
            }
        }
        
        return neighbors;
    }

    void GridAStar::generate_all_steps()
    {
        if (m_rows == 0 || m_cols == 0)
        {
            LOG_WARNING(
                "Cannot generate Grid A* steps: grid not initialized"
            );
            return;
        }
        
        if (m_start.row < 0 || m_start.col < 0 || 
            m_target.row < 0 || m_target.col < 0)
        {
            LOG_ERROR("Invalid start or target position");
            return;
        }
        
        m_steps.clear();
        m_visited_order.clear();
        m_frontier_order.clear();
        
        // Initialize cells
        m_cells.assign(m_rows, std::vector<GridCell>(m_cols));
        for (int row = 0; row < m_rows; ++row)
        {
            for (int col = 0; col < m_cols; ++col)
            {
                m_cells[row][col] = GridCell(row, col);
                m_cells[row][col].weight = get_cell_weight(
                    GridPosition(row, col)
                );
            }
        }
        
        // Initialize state
        GridAStarState state;
        state.phase = GridAStarState::Phase::INITIALIZE;
        state.explored_count = 0;
        state.comparisons = 0;
        state.is_complete = false;
        state.target_found = false;
        
        // Initialization
        push_step(state, "init");
        
        // Push start node to open set
        m_cells[m_start.row][m_start.col].g_score = 0.0f;
        m_cells[m_start.row][m_start.col].f_score = calculate_heuristic(
            m_start, m_target
        );
        state.open_set.emplace(
            m_cells[m_start.row][m_start.col].f_score, 
            m_start
        );
        m_cells[m_start.row][m_start.col].in_open = true;
        m_frontier_order.push_back(m_start);

        state.new_f_score = m_cells[m_start.row][m_start.col].f_score;
        
        state.phase = GridAStarState::Phase::PUSH_START;
        push_step(state, "push_start");
        
        // Process A* until complete or target found
        while (!state.is_complete && 
               !state.target_found && 
               !state.open_set.empty())
        {
            // Pop node with minimum f-score
            auto [current_f, current_pos] = state.open_set.top();
            state.open_set.pop();
            m_cells[current_pos.row][current_pos.col].in_open = false;
            state.current_pos = current_pos;
            
            state.phase = GridAStarState::Phase::POP_MIN;
            push_step(state, "pop_min");
            
            // Goal check
            state.phase = GridAStarState::Phase::GOAL_CHECK;
            push_step(state, "goal_check");
            
            if (current_pos == m_target)
            {
                state.target_found = true;

                reconstruct_path();

                state.phase = GridAStarState::Phase::TARGET_FOUND;
                push_step(state, "target_found");

                break;
            }
            
            // Adding to closed set
            m_cells[current_pos.row][current_pos.col].visited = true;
            state.explored_count++;
            m_visited_order.push_back(current_pos);
            
            state.phase = GridAStarState::Phase::ADD_TO_CLOSED;
            push_step(state, "add_to_closed");
            
            // Evaluating all neighbors
            auto neighbors = get_neighbors(current_pos);
            
            for (const auto& neighbor : neighbors)
            {
                state.current_neighbor = neighbor;
                state.comparisons++;
                
                state.phase = GridAStarState::Phase::EVALUATE_NEIGHBOR;
                push_step(state, "evaluate_neighbor");
                
                // Skip if in closed set
                if (m_cells[neighbor.row][neighbor.col].visited)
                {
                    continue;
                }
                
                float move_cost = get_movement_cost(current_pos, neighbor);
                state.current_edge_weight = move_cost;
                float tentative_g = m_cells[current_pos.row][current_pos.col].g_score + move_cost;
                state.tentative_g_score = tentative_g;
                state.old_g_score = m_cells[neighbor.row][neighbor.col].g_score;
                
                state.phase = GridAStarState::Phase::RELAX_EDGE;
                push_step(state, "relax_edge");
                
                if (tentative_g < m_cells[neighbor.row][neighbor.col].g_score)
                {
                    m_cells[neighbor.row][neighbor.col].parent = current_pos;
                    m_cells[neighbor.row][neighbor.col].g_score = tentative_g;
                    float h_score = calculate_heuristic(neighbor, m_target);
                    float new_f = tentative_g + h_score;
                    m_cells[neighbor.row][neighbor.col].f_score = new_f;
                    state.new_f_score = new_f;
                    
                    state.phase = GridAStarState::Phase::UPDATE_SCORES;
                    push_step(state, "update_scores");
                    
                    if (!m_cells[neighbor.row][neighbor.col].in_open)
                    {
                        state.open_set.emplace(new_f, neighbor);
                        m_cells[neighbor.row][neighbor.col].in_open = true;
                        m_frontier_order.push_back(neighbor);
                        
                        state.phase = GridAStarState::Phase::PUSH_NEIGHBOR;
                        push_step(state, "push_neighbor");
                    }
                }
            }
            
            state.current_neighbor = GridPosition(-1, -1);
        }
        
        // Final steps
        if (state.open_set.empty() && !state.target_found)
        {
            state.is_complete = true;
            state.phase = GridAStarState::Phase::OPEN_SET_EMPTY;
            push_step(state, "open_set_empty");
        }
        
        // Reconstruct path if target found
        if (state.target_found)
        {
            reconstruct_path();
        }
        
        state.is_complete = true;
        state.phase = GridAStarState::Phase::COMPLETED;
        push_step(state, "completed");
        
        LOG_DEBUG("Generated {} steps for Grid A*", m_steps.size());
    }

    void GridAStar::push_step(
        const GridAStarState& state, 
        const std::string& operation_id)
    {
        auto step = create_step_from_state(state, operation_id);
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep GridAStar::create_step_from_state(
        const GridAStarState& state, 
        const std::string& operation_id
    ) const
    {
        AlgorithmStep step;
        
        // Flatten grid data for visualization
        step.data.clear();
        for (int row = 0; row < m_rows; ++row)
        {
            for (int col = 0; col < m_cols; ++col)
            {
                int val = static_cast<int>(m_grid[row][col]);
                step.data.push_back(val);
            }
        }
        
        step.metadata.operation_id = operation_id;
        populate_step_metadata(
            step, 
            state, 
            operation_id
        );
        step.description = format_step_description(
            operation_id, 
            step
        );
        update_visualization_data(
            step, 
            state, 
            operation_id
        );
        
        return step;
    }

    void GridAStar::populate_step_metadata(
        AlgorithmStep& step, 
        const GridAStarState& state, 
        const std::string& operation_id
    ) const
    {
        step.metadata.set(
            "rows", 
            m_rows, 
            "Number of rows in grid"
        );
        step.metadata.set(
            "cols", 
            m_cols, 
            "Number of columns in grid"
        );
        step.metadata.set(
            "start_row", 
            m_start.row, 
            "Start row"
        );
        step.metadata.set(
            "start_col", 
            m_start.col, 
            "Start column"
        );
        step.metadata.set(
            "target_row", 
            m_target.row, 
            "Target row"
        );
        step.metadata.set(
            "target_col", 
            m_target.col, 
            "Target column"
        );
        step.metadata.set(
            "open_set_size", 
            state.open_set.size(), 
            "Current open set size"
        );
        step.metadata.set(
            "closed_set_size", 
            state.explored_count, 
            "Number of cells explored"
        );
        step.metadata.set(
            "comparisons", 
            state.comparisons, 
            "Number of neighbor evaluations"
        );
        step.metadata.set(
            "f_score", 
            "N/A", 
            "F-score"
        );
        if (std::isfinite(state.new_f_score))
        {
            step.metadata.set(
                "f_score",
                state.new_f_score,
                "F-score"
            );
        }

        if (state.current_pos.row >= 0)
        {
            step.metadata.set(
                "current_row", 
                state.current_pos.row, 
                "Current row"
            );
            step.metadata.set(
                "current_col", 
                state.current_pos.col, 
                "Current column"
            );
            
            if (std::isfinite(m_cells[state.current_pos.row][state.current_pos.col].g_score))
            {
                step.metadata.set(
                    "g_score", 
                    m_cells[state.current_pos.row][state.current_pos.col].g_score,
                    "Current g-score"
                );
            }
            if (std::isfinite(m_cells[state.current_pos.row][state.current_pos.col].f_score))
            {
                step.metadata.set(
                    "f_score", 
                    m_cells[state.current_pos.row][state.current_pos.col].f_score,
                    "Current f-score"
                );
            }
        }
        
        if (state.current_neighbor.row >= 0)
        {
            step.metadata.set(
                "neighbor_row", 
                state.current_neighbor.row, 
                "Neighbor row"
            );
            step.metadata.set("neighbor_col", 
                state.current_neighbor.col, 
                "Neighbor column"
            );
            step.metadata.set("edge_weight", 
                state.current_edge_weight, 
                "Movement cost to neighbor"
            );
            
            if (std::isfinite(state.tentative_g_score))
            {
                step.metadata.set(
                    "tentative_g", 
                    state.tentative_g_score, 
                    "Potential new g-score"
                );
            }
            if (std::isfinite(state.old_g_score))
            {
                step.metadata.set(
                    "old_g", 
                    state.old_g_score, 
                    "Current g-score of neighbor"
                );
            } else
            {
                step.metadata.set(
                    "old_g", 
                    "INFINITY", 
                    "Current g-score of neighbor"
                );
            }
            if (std::isfinite(state.tentative_g_score))
            {
                step.metadata.set(
                    "new_g",
                    state.tentative_g_score,
                    "New g-score for neighbor"
                );
            }

            if (std::isfinite(state.new_f_score))
            {
                step.metadata.set(
                    "new_f",
                    state.new_f_score,
                    "New f-score for neighbor"
                );
            }
        }
        
        if (state.target_found)
        {
            step.metadata.set(
                "path_length",
                m_path.size(),
                "Length of found path"
            );

            float path_cost = m_cells[m_target.row][m_target.col].g_score;

            if (std::isfinite(path_cost))
            {
                step.metadata.set(
                    "path_cost",
                    path_cost,
                    "Total path cost"
                );
            }

        }
        
        // Phase as string
        std::string phase_str;
        switch (state.phase)
        {
            case GridAStarState::Phase::INITIALIZE: 
                phase_str = "Initializing"; break;
            case GridAStarState::Phase::PUSH_START: 
                phase_str = "Pushing Start Cell"; break;
            case GridAStarState::Phase::POP_MIN: 
                phase_str = "Popping Minimum F-Score"; break;
            case GridAStarState::Phase::GOAL_CHECK: 
                phase_str = "Goal Check"; break;
            case GridAStarState::Phase::ADD_TO_CLOSED: 
                phase_str = "Adding to Closed Set"; break;
            case GridAStarState::Phase::EVALUATE_NEIGHBOR: 
                phase_str = "Evaluating Neighbor"; break;
            case GridAStarState::Phase::RELAX_EDGE: 
                phase_str = "Relaxing Edge"; break;
            case GridAStarState::Phase::UPDATE_SCORES: 
                phase_str = "Updating Scores"; break;
            case GridAStarState::Phase::PUSH_NEIGHBOR: 
                phase_str = "Pushing to Open Set"; break;
            case GridAStarState::Phase::TARGET_FOUND: 
                phase_str = "Target Found!"; break;
            case GridAStarState::Phase::OPEN_SET_EMPTY: 
                phase_str = "Open Set Empty - No Path"; break;
            case GridAStarState::Phase::COMPLETED: 
                phase_str = "Completed"; break;
        }
        step.metadata.set(
            "phase", 
            phase_str, 
            "Current Grid A* phase"
        );
    }

    void GridAStar::update_visualization_data(
        AlgorithmStep& step, 
        const GridAStarState& state, 
        const std::string& operation_id
    ) const
    {
        auto& viz = step.visualization;
        
        // For grid-based visualization, we'll use the graph_state fields to communicate
        // which cells are visited, frontier, and path
        viz.graph_state.visited_nodes.clear();
        viz.graph_state.frontier_nodes.clear();
        viz.graph_state.path.clear();
        viz.graph_state.node_distances.clear();
        
        // Convert 2D grid positions to 1D node IDs for compatibility with existing visualizer
        auto to_node_id = [this](int row, int col) -> size_t {
            return row * m_cols + col;
        };
        
        // Mark visited cells (closed set)
        for (int row = 0; row < m_rows; ++row)
        {
            for (int col = 0; col < m_cols; ++col)
            {
                if (m_cells[row][col].visited)
                {
                    viz.graph_state.visited_nodes.push_back(
                        to_node_id(row, col)
                    );
                    if (std::isfinite(m_cells[row][col].g_score))
                    {
                        viz.graph_state.node_distances[to_node_id(row, col)] = 
                            static_cast<int>(m_cells[row][col].g_score);
                    }
                }
            }
        }
        
        // Mark frontier cells (open set)
        // We need to track which positions are in open_set
        for (int row = 0; row < m_rows; ++row)
            for (int col = 0; col < m_cols; ++col)
                if (m_cells[row][col].in_open)
                    viz.graph_state.frontier_nodes.push_back(
                        to_node_id(row, col)
                    );
        
        // Mark current node
        if (state.current_pos.row >= 0)
        {
            viz.highlighted_index = to_node_id(
                state.current_pos.row, 
                state.current_pos.col
            );
        }
        
        // Build path
        for (const auto& pos : m_path)
        {
            viz.graph_state.path.push_back(
                to_node_id(pos.row, pos.col)
            );
        }
        
        // Mark active edge (current neighbor being evaluated)
        if (state.current_neighbor.row >= 0 && 
            state.current_pos.row >= 0)
        {
            viz.graph_state.active_edges.emplace_back(
                to_node_id(state.current_pos.row, state.current_pos.col),
                to_node_id(state.current_neighbor.row, state.current_neighbor.col)
            );
            viz.compared_index = to_node_id(
                state.current_pos.row, 
                state.current_pos.col
            );
            viz.additional_highlights.push_back(
                to_node_id(state.current_neighbor.row, state.current_neighbor.col)
            );
        }
        
        // Store terrain weights as distances for visualization
        for (int row = 0; row < m_rows; ++row)
        {
            for (int col = 0; col < m_cols; ++col)
            {
                int weight = get_cell_weight(GridPosition(row, col));
                if (weight > 1 && weight <= 5)
                {
                    viz.graph_state.node_distances[to_node_id(row, col)] = weight;
                }
            }
        }
        
        viz.comparison_count = state.comparisons;
        viz.explored_node_count = state.explored_count;
        viz.visited_node_count = viz.graph_state.visited_nodes.size();
        viz.is_complete = state.is_complete || state.target_found;
    }

    void GridAStar::reset_state()
    {
        m_current_state = GridAStarState{};
        m_path.clear();
        m_visited_order.clear();
        m_frontier_order.clear();
        
        if (m_rows > 0 && m_cols > 0)
        {
            m_cells.assign(m_rows, std::vector<GridCell>(m_cols));
            for (int row = 0; row < m_rows; ++row)
            {
                for (int col = 0; col < m_cols; ++col)
                {
                    m_cells[row][col] = GridCell(row, col);
                    m_cells[row][col].weight = get_cell_weight(GridPosition(row, col));
                }
            }
        }
    }

} // namespace c2l::algorithms