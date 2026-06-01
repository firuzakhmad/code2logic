//
// Created by Akhmad on 5/20/26.
//

#include "algorithms/theta_star.hpp"
#include "core/utils/logger/logger.hpp"

#include <algorithm>
#include <cmath>
#include <queue>

namespace c2l::algorithms
{
    ThetaStar::ThetaStar(core::JsonConfigManager& json_config_manager)
        : GridPathfindingBase(json_config_manager, AlgorithmType::THETA_STAR)
    {
        LOG_DEBUG("Theta* created with default 20x20 grid");
    }

    float ThetaStar::calculate_heuristic(
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

    float ThetaStar::get_euclidean_distance(
        const GridPosition& from, 
        const GridPosition& to
    ) const
    {
        float dx = static_cast<float>(from.col - to.col);
        float dy = static_cast<float>(from.row - to.row);
        return std::sqrt(dx * dx + dy * dy);
    }

    bool ThetaStar::has_line_of_sight(
        const GridPosition& from, 
        const GridPosition& to
    ) const
    {
        if (from == to) return true;

        int x0 = from.col;
        int y0 = from.row;
        int x1 = to.col;
        int y1 = to.row;

        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;

        int x = x0;
        int y = y0;

        while (true)
        {
            if (x == x1 && y == y1)
                break;

            // Check if current cell is a wall (excluding start and target)
            if ((x != x0 || y != y0) && (x != x1 || y != y1))
            {
                if (!is_walkable(y, x))
                    return false;
            }

            int e2 = 2 * err;
            if (e2 > -dy)
            {
                err -= dy;
                x += sx;
            }
            if (e2 < dx)
            {
                err += dx;
                y += sy;
            }
        }

        return true;
    }

    float ThetaStar::get_movement_cost(
        const GridPosition& from, 
        const GridPosition& to
    ) const
    {
        return get_euclidean_distance(from, to);
    }

    std::vector<GridPosition> ThetaStar::get_neighbors(
        const GridPosition& pos
    ) const
    {
        std::vector<GridPosition> neighbors;

        // 8-directional movement
        for (int i = 0; i < 8; ++i)
        {
            int nr = pos.row + DIRS[i][0];
            int nc = pos.col + DIRS[i][1];

            if (is_walkable(nr, nc))
            {
                // For diagonal moves, check if we're cutting corners
                if (DIRS[i][0] != 0 && DIRS[i][1] != 0)
                {
                    bool can_move_diag = true;
                    if (!is_walkable(nr, pos.col) || !is_walkable(pos.row, nc))
                    {
                        can_move_diag = false;
                    }
                    if (can_move_diag)
                    {
                        neighbors.emplace_back(nr, nc);
                    }
                }
                else
                {
                    neighbors.emplace_back(nr, nc);
                }
            }
        }

        return neighbors;
    }

    void ThetaStar::generate_all_steps()
    {
        if (m_rows == 0 || m_cols == 0)
        {
            LOG_WARNING(
                "Cannot generate Theta* steps: grid not initialized"
            );
            ThetaState empty_state;
            empty_state.is_complete = true;
            empty_state.phase = ThetaState::Phase::COMPLETED;
            push_step(empty_state, "error_no_grid");
            return;
        }

        if (m_start.row < 0 || m_start.col < 0 || 
            m_target.row < 0 || m_target.col < 0)
        {
            LOG_ERROR(
                "Invalid start or target position"
            );
            ThetaState empty_state;
            empty_state.is_complete = true;
            empty_state.phase = ThetaState::Phase::COMPLETED;
            push_step(empty_state, "error_invalid_positions");
            return;
        }

        m_steps.clear();
        m_visited_order.clear();
        m_frontier_order.clear();

        // Initialization cells
        m_cells.assign(m_rows, std::vector<GridCell>(m_cols));
        for (int row = 0; row < m_rows; ++row)
        {
            for (int col = 0; col < m_cols; ++col)
            {
                m_cells[row][col] = GridCell(row, col);
                m_cells[row][col].weight = get_cell_weight(GridPosition(row, col));
            }
        }

        // Initialize state
        ThetaState state;
        state.phase = ThetaState::Phase::INITIALIZE;
        state.explored_count = 0;
        state.comparisons = 0;
        state.line_of_sight_checks = 0;
        state.is_complete = false;
        state.target_found = false;

        push_step(state, "init");

        // Initialize start node
        m_cells[m_start.row][m_start.col].g_score = 0.0f;
        m_cells[m_start.row][m_start.col].f_score = calculate_heuristic(
            m_start, m_target
        );
        m_cells[m_start.row][m_start.col].parent = m_start;
        state.open_set.emplace(
            m_cells[m_start.row][m_start.col].f_score, 
            m_start
        );
        m_cells[m_start.row][m_start.col].in_open = true;
        m_frontier_order.push_back(m_start);

        state.phase = ThetaState::Phase::PUSH_START;
        push_step(state, "push_start");

        // Process Theta* until complete
        while (!state.is_complete && 
               !state.target_found && 
               !state.open_set.empty())
        {
            auto [current_f, current_pos] = state.open_set.top();
            state.open_set.pop();
            m_cells[current_pos.row][current_pos.col].in_open = false;
            state.current_pos = current_pos;

            state.phase = ThetaState::Phase::POP_MIN;
            push_step(state, "pop_min");

            // Goal check
            state.phase = ThetaState::Phase::GOAL_CHECK;
            push_step(state, "goal_check");

            if (current_pos == m_target)
            {
                state.target_found = true;
                reconstruct_path();
                state.phase = ThetaState::Phase::TARGET_FOUND;
                push_step(state, "target_found");
                break;
            }

            // Add to closed set
            if (!m_cells[current_pos.row][current_pos.col].visited)
            {
                m_cells[current_pos.row][current_pos.col].visited = true;
                state.explored_count++;
                m_visited_order.push_back(current_pos);
            }

            state.phase = ThetaState::Phase::ADD_TO_CLOSED;
            push_step(state, "add_to_closed");

            // Evaluate all neighbors
            auto neighbors = get_neighbors(current_pos);

            for (const auto& neighbor : neighbors)
            {
                state.current_neighbor = neighbor;
                state.comparisons++;
                state.line_of_sight_success = false;

                state.phase = ThetaState::Phase::EVALUATE_NEIGHBOR;
                push_step(state, "evaluate_neighbor");

                // Skip if in closed set
                if (m_cells[neighbor.row][neighbor.col].visited)
                    continue;

                // Theta* key difference: Check line of sight to parent
                GridPosition parent = m_cells[current_pos.row][current_pos.col].parent;
                bool line_of_sight = false;
                float tentative_g = core::INF;

                if (m_enable_line_of_sight && 
                    parent.row >= 0 && parent.col >= 0 && 
                    parent != current_pos)
                {
                    state.line_of_sight_checks++;

                    line_of_sight = has_line_of_sight(parent, neighbor);
                    state.line_of_sight_success = line_of_sight;

                    state.phase = ThetaState::Phase::LINE_OF_SIGHT_CHECK;
                    push_step(state, "line_of_sight_check");

                    if (line_of_sight)
                    {
                        state.line_of_sight_parent = parent;
                        // Path through parent (any-angle)
                        tentative_g = m_cells[parent.row][parent.col].g_score +
                                      get_movement_cost(parent, neighbor);

                        state.phase = ThetaState::Phase::UPDATE_WITH_PARENT;
                        push_step(state, "update_with_parent");
                    }
                }

                if (!line_of_sight)
                {
                    // Path through current node (grid-based)
                    tentative_g = m_cells[current_pos.row][current_pos.col].g_score +
                                  get_movement_cost(current_pos, neighbor);
                    state.line_of_sight_parent = GridPosition(-1, -1);
                }

                state.tentative_g_score = tentative_g;
                state.old_g_score = m_cells[neighbor.row][neighbor.col].g_score;
                state.current_edge_weight = get_movement_cost(current_pos, neighbor);

                state.phase = ThetaState::Phase::RELAX_EDGE;
                push_step(state, "relax_edge");

                if (tentative_g < m_cells[neighbor.row][neighbor.col].g_score)
                {
                    if (line_of_sight)
                    {
                        m_cells[neighbor.row][neighbor.col].parent = parent;
                    }
                    else
                    {
                        m_cells[neighbor.row][neighbor.col].parent = current_pos;
                    }

                    m_cells[neighbor.row][neighbor.col].g_score = tentative_g;
                    float h_score = calculate_heuristic(neighbor, m_target);
                    float new_f = tentative_g + h_score;
                    m_cells[neighbor.row][neighbor.col].f_score = new_f;
                    state.new_f_score = new_f;

                    state.phase = ThetaState::Phase::UPDATE_SCORES;
                    push_step(state, "update_scores");

                    if (!m_cells[neighbor.row][neighbor.col].in_open)
                    {
                        state.open_set.emplace(new_f, neighbor);
                        m_cells[neighbor.row][neighbor.col].in_open = true;
                        m_frontier_order.push_back(neighbor);

                        state.phase = ThetaState::Phase::PUSH_NEIGHBOR;
                        push_step(state, "push_neighbor");
                    }
                }
            }

            state.current_neighbor = GridPosition(-1, -1);
            state.line_of_sight_parent = GridPosition(-1, -1);
        }

        // Final steps
        if (state.open_set.empty() && !state.target_found)
        {
            state.is_complete = true;
            state.phase = ThetaState::Phase::OPEN_SET_EMPTY;
            push_step(state, "open_set_empty");
        }

        if (state.target_found)
        {
            reconstruct_path();
        }

        state.is_complete = true;
        state.phase = ThetaState::Phase::COMPLETED;
        push_step(state, "completed");

        LOG_DEBUG("Generated {} steps for Theta* with {} line-of-sight checks",
                  m_steps.size(), state.line_of_sight_checks);
    }

    void ThetaStar::push_step(
        const ThetaState& state, 
        const std::string& operation_id)
    {
        auto step = create_step_from_state(state, operation_id);
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep ThetaStar::create_step_from_state(
        const ThetaState& state, 
        const std::string& operation_id
    ) const
    {
        AlgorithmStep step;

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
        populate_step_metadata(step, state, operation_id);
        step.description = format_step_description(operation_id, step);
        update_visualization_data(step, state, operation_id);

        return step;
    }

    void ThetaStar::populate_step_metadata(
        AlgorithmStep& step, 
        const ThetaState& state, 
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
            "line_of_sight_checks", 
            state.line_of_sight_checks, 
            "Number of LOS checks performed"
        );
        step.metadata.set(
            "old_g", 
            state.old_g_score, 
            "Current g-score of neighbor"
        );
        step.metadata.set(
            "new_g", 
            state.tentative_g_score, 
            "Updated g-score"
        );
        step.metadata.set(
            "los_checks",
            state.line_of_sight_checks,
            "Number of LOS checks"
        );
        step.metadata.set(
            "f_score", 
            -1.0f, 
            "Current f-score"
        );
        step.metadata.set(
            "parent_row", 
            -1, 
            "Parent row"
        );
        step.metadata.set(
            "parent_col", 
            -1, 
            "Parent column"
        );
        step.metadata.set(
            "los_checks", 
            0, 
            "LOS checks"
        );


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

            if (m_cells[state.current_pos.row][state.current_pos.col].g_score < core::INF)
            {
                step.metadata.set(
                    "g_score", 
                    m_cells[state.current_pos.row][state.current_pos.col].g_score, 
                    "Current g-score"
                );
            }
            if (m_cells[state.current_pos.row][state.current_pos.col].f_score < core::INF)
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
            step.metadata.set(
                "neighbor_col", 
                state.current_neighbor.col, 
                "Neighbor column"
            );
            step.metadata.set(
                "edge_weight", 
                state.current_edge_weight, 
                "Movement cost to neighbor"
            );
            step.metadata.set(
                "line_of_sight", 
                state.line_of_sight_success ? "Yes" : "No", 
                "Line of sight to parent?"
            );

            if (state.line_of_sight_parent.row >= 0)
            {
                step.metadata.set(
                    "parent_row", 
                    state.line_of_sight_parent.row, 
                    "LOS parent row"
                );
                step.metadata.set(
                    "parent_col", 
                    state.line_of_sight_parent.col, 
                    "LOS parent column"
                );

                // compatibility aliases
                step.metadata.set(
                    "parent_row", 
                    state.line_of_sight_parent.row, 
                    "Parent row"
                );
                step.metadata.set(
                    "parent_col", 
                    state.line_of_sight_parent.col, 
                    "Parent col"
                );
            }

            if (std::isfinite(state.tentative_g_score))
            {
                step.metadata.set(
                    "tentative_g", 
                    state.tentative_g_score, 
                    "Potential new g-score"
                );
                step.metadata.set(
                    "new_g",
                    state.tentative_g_score,
                    "Updated g-score"
                );
            }
            if (std::isfinite(state.old_g_score))
            {
                step.metadata.set(
                    "old_g", 
                    state.old_g_score, 
                    "Current g-score of neighbor"
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
            if (path_cost < core::INF)
            {
                step.metadata.set(
                    "path_cost", 
                    path_cost, 
                    "Total path cost"
                );
            }
        }

        std::string phase_str;
        switch (state.phase)
        {
            case ThetaState::Phase::INITIALIZE: 
                phase_str = "Initializing"; break;
            case ThetaState::Phase::PUSH_START: 
                phase_str = "Pushing Start Cell"; break;
            case ThetaState::Phase::POP_MIN: 
                phase_str = "Popping Minimum F-Score"; break;
            case ThetaState::Phase::GOAL_CHECK: 
                phase_str = "Goal Check"; break;
            case ThetaState::Phase::ADD_TO_CLOSED: 
                phase_str = "Adding to Closed Set"; break;
            case ThetaState::Phase::EVALUATE_NEIGHBOR: 
                phase_str = "Evaluating Neighbor"; break;
            case ThetaState::Phase::LINE_OF_SIGHT_CHECK: 
                phase_str = "Line of Sight Check"; break;
            case ThetaState::Phase::UPDATE_WITH_PARENT: 
                phase_str = "Updating with Parent (Any-Angle)"; break;
            case ThetaState::Phase::RELAX_EDGE: 
                phase_str = "Relaxing Edge"; break;
            case ThetaState::Phase::UPDATE_SCORES: 
                phase_str = "Updating Scores"; break;
            case ThetaState::Phase::PUSH_NEIGHBOR: 
                phase_str = "Pushing to Open Set"; break;
            case ThetaState::Phase::TARGET_FOUND: 
                phase_str = "Target Found!"; break;
            case ThetaState::Phase::OPEN_SET_EMPTY: 
                phase_str = "Open Set Empty - No Path"; break;
            case ThetaState::Phase::COMPLETED: 
                phase_str = "Completed"; break;
        }
        step.metadata.set(
            "phase", 
            phase_str, 
            "Current Theta* phase"
        );
    }

    void ThetaStar::update_visualization_data(
        AlgorithmStep& step, 
        const ThetaState& state, 
        const std::string& operation_id
    ) const
    {
        auto& viz = step.visualization;

        viz.graph_state.visited_nodes.clear();
        viz.graph_state.frontier_nodes.clear();
        viz.graph_state.path.clear();
        viz.graph_state.node_distances.clear();

        auto to_node_id = [this](int row, int col) -> size_t {
            return row * m_cols + col;
        };

        // Mark visited cells
        for (int row = 0; row < m_rows; ++row)
        {
            for (int col = 0; col < m_cols; ++col)
            {
                if (m_cells[row][col].visited)
                {
                    viz.graph_state.visited_nodes.push_back(
                        to_node_id(row, col)
                    );
                    if (m_cells[row][col].g_score < core::INF)
                    {
                        viz.graph_state.node_distances[to_node_id(row, col)] =
                            static_cast<int>(m_cells[row][col].g_score * 10);
                    }
                }
            }
        }

        // Marking frontier cells
        for (int row = 0; row < m_rows; ++row)
        {
            for (int col = 0; col < m_cols; ++col)
            {
                if (m_cells[row][col].in_open && !m_cells[row][col].visited)
                {
                    viz.graph_state.frontier_nodes.push_back(
                        to_node_id(row, col)
                    );
                }
            }
        }

        // Marking current node
        if (state.current_pos.row >= 0)
        {
            viz.highlighted_index = to_node_id(
                state.current_pos.row, 
                state.current_pos.col
            );
        }

        // Building path (the any-angle path)
        for (const auto& pos : m_path)
        {
            viz.graph_state.path.push_back(
                to_node_id(pos.row, pos.col)
            );
        }

        // Marking line of sight edge
        if (state.line_of_sight_parent.row >= 0 && state.current_neighbor.row >= 0)
        {
            viz.graph_state.active_edges.emplace_back(
                to_node_id(state.line_of_sight_parent.row, state.line_of_sight_parent.col),
                to_node_id(state.current_neighbor.row, state.current_neighbor.col)
            );
            viz.additional_highlights.push_back(
                to_node_id(state.current_neighbor.row, state.current_neighbor.col)
            );
        }
        else if (state.current_neighbor.row >= 0 && state.current_pos.row >= 0)
        {
            viz.graph_state.active_edges.emplace_back(
                to_node_id(state.current_pos.row, state.current_pos.col),
                to_node_id(state.current_neighbor.row, state.current_neighbor.col)
            );
            viz.additional_highlights.push_back(
                to_node_id(state.current_neighbor.row, state.current_neighbor.col)
            );
        }

        viz.comparison_count = state.comparisons;
        viz.visited_node_count = viz.graph_state.visited_nodes.size();
        viz.is_complete = state.is_complete || state.target_found;
    }

    void ThetaStar::reset_state()
    {
        m_current_state = ThetaState{};
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