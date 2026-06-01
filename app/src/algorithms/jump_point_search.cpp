#include "algorithms/jump_point_search.hpp"
#include "core/utils/logger/logger.hpp"
#include "core/utils/variables.hpp"

#include <algorithm>
#include <sstream>
#include <cmath>
#include <queue>
#include <unordered_set>

namespace c2l::algorithms
{
    JumpPointSearch::JumpPointSearch(core::JsonConfigManager& json_config_manager)
        : GridPathfindingBase(json_config_manager, AlgorithmType::JUMP_POINT_SEARCH)
    {
        LOG_DEBUG("Jump Point Search created and metadata loaded from JSON");
    }

    float JumpPointSearch::calculate_heuristic(
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

    float JumpPointSearch::get_movement_cost(
        const GridPosition& from, 
        const GridPosition& to
    ) const
    {
        bool is_diagonal = (from.row != to.row && from.col != to.col);
        return is_diagonal && m_allow_diagonals ? static_cast<float>(core::SQRT2) : 1.0f;
    }

    bool JumpPointSearch::has_forced_neighbor(
        const GridPosition& pos,
        const GridPosition& dir
    ) const
    {
        int dx = dir.col;
        int dy = dir.row;

        // Diagonal
        if (dx != 0 && dy != 0)
        {
            if ((!is_walkable(pos.row - dy, pos.col) &&
                 is_walkable(pos.row - dy, pos.col + dx)) ||

                (!is_walkable(pos.row, pos.col - dx) &&
                 is_walkable(pos.row + dy, pos.col - dx)))
            {
                return true;
            }
        }
        // Horizontal
        else if (dx != 0)
        {
            if ((!is_walkable(pos.row + 1, pos.col) &&
                 is_walkable(pos.row + 1, pos.col + dx)) ||

                (!is_walkable(pos.row - 1, pos.col) &&
                 is_walkable(pos.row - 1, pos.col + dx)))
            {
                return true;
            }
        }
        // Vertical
        else
        {
            if ((!is_walkable(pos.row, pos.col + 1) &&
                 is_walkable(pos.row + dy, pos.col + 1)) ||

                (!is_walkable(pos.row, pos.col - 1) &&
                 is_walkable(pos.row + dy, pos.col - 1)))
            {
                return true;
            }
        }

        return false;
    }

    GridPosition JumpPointSearch::jump(
        const GridPosition& from,
        const GridPosition& direction,
        const GridPosition& target)
    {
        GridPosition current = from;

        while (true)
        {
            if (direction.row != 0 && direction.col != 0)
            {
                if (!is_walkable(current.row + direction.row, current.col) ||
                    !is_walkable(current.row, current.col + direction.col))
                {
                    return GridPosition(-1, -1);
                }
            }

            GridPosition next(current.row + direction.row, current.col + direction.col);

            // Out of bounds or wall
            if (!is_walkable(next.row, next.col))
                return GridPosition(-1, -1);

            // Found target
            if (next == target)
                return next;

            // Jump point: has forced neighbors
            if (has_forced_neighbor(next, direction))
                return next;

            // Diagonal movement: check cardinal components recursively
            // (These stay recursive but are bounded by the cardinal loop above)
            if (direction.row != 0 && direction.col != 0)
            {
                GridPosition h = jump(next, GridPosition(0, direction.col), target);
                GridPosition v = jump(next, GridPosition(direction.row, 0), target);
                if (h.row != -1 || v.row != -1)
                    return next;
            }

            current = next;
        }
    }

    std::vector<GridPosition> JumpPointSearch::get_pruned_directions(
        const GridPosition& pos,
        const GridPosition& parent
    ) const
    {
        std::vector<GridPosition> dirs;

        // Start node: all directions
        if (parent.row == -1)
        {
            for (int i = 0; i < (m_allow_diagonals ? 8 : 4); ++i)
            {
                dirs.emplace_back(DIRS[i][0], DIRS[i][1]);
            }
            return dirs;
        }

        int dx = pos.col - parent.col;
        int dy = pos.row - parent.row;

        dx = (dx == 0) ? 0 : dx / std::abs(dx);
        dy = (dy == 0) ? 0 : dy / std::abs(dy);

        // Diagonal movement
        if (dx != 0 && dy != 0)
        {
            // Natural neighbors
            dirs.emplace_back(dy, dx);
            dirs.emplace_back(dy, 0);
            dirs.emplace_back(0, dx);

            // Forced neighbors
            if (!is_walkable(pos.row - dy, pos.col))
                dirs.emplace_back(-dy, dx);

            if (!is_walkable(pos.row, pos.col - dx))
                dirs.emplace_back(dy, -dx);
        }
        // Horizontal
        else if (dx != 0)
        {
            dirs.emplace_back(0, dx);

            if (!is_walkable(pos.row + 1, pos.col))
                dirs.emplace_back(1, dx);

            if (!is_walkable(pos.row - 1, pos.col))
                dirs.emplace_back(-1, dx);
        }
        // Vertical
        else
        {
            dirs.emplace_back(dy, 0);

            if (!is_walkable(pos.row, pos.col + 1))
                dirs.emplace_back(dy, 1);

            if (!is_walkable(pos.row, pos.col - 1))
                dirs.emplace_back(dy, -1);
        }

        return dirs;
    }

    std::vector<GridPosition> JumpPointSearch::get_pruned_neighbors(
        const GridPosition& pos,
        const GridPosition& parent)
    {
        std::vector<GridPosition> neighbors;
        auto dirs = get_pruned_directions(pos, parent);

        for (const auto& dir : dirs)
        {
            GridPosition jump_point = jump(pos, dir, m_target);
            if (jump_point.row != -1 && jump_point.col != -1)
            {
                neighbors.push_back(jump_point);
            }
        }

        return neighbors;
    }

    void JumpPointSearch::generate_all_steps()
    {
        if (m_rows == 0 || m_cols == 0)
        {
            LOG_WARNING(
                "Cannot generate JPS steps: grid not initialized"
            );
            return;
        }

        if (m_start.row < 0 || m_start.col < 0 || 
            m_target.row < 0 || m_target.col < 0)
        {
            LOG_ERROR("Invalid start or target position");
            return;
        }

        if (!is_walkable(m_start.row, m_start.col) ||
            !is_walkable(m_target.row, m_target.col))
        {
            LOG_ERROR("Start or target is blocked");
            return;
        }


        m_steps.clear();
        m_visited_order.clear();
        m_jump_points.clear();

        // Initialize cells
        m_cells.assign(m_rows, std::vector<GridCell>(m_cols));
        for (int row = 0; row < m_rows; ++row)
        {
            for (int col = 0; col < m_cols; ++col)
            {
                m_cells[row][col] = GridCell(row, col);
            }
        }

        // Initialization state
        JPSState state;
        state.phase = JPSState::Phase::INITIALIZE;
        state.explored_count = 0;
        state.comparisons = 0;
        state.jump_count = 0;
        state.is_complete = false;
        state.target_found = false;

        push_step(state, "init");

        // Pushing start node to open set
        m_cells[m_start.row][m_start.col].g_score = 0.0f;
        m_cells[m_start.row][m_start.col].f_score = calculate_heuristic(
            m_start, 
            m_target
        );
        state.open_set.emplace(
            m_cells[m_start.row][m_start.col].f_score, m_start
        );
        m_cells[m_start.row][m_start.col].in_open = true;
        m_cells[m_start.row][m_start.col].is_jump_point = true;
        m_jump_points.push_back(m_start);

        state.phase = JPSState::Phase::PUSH_START;
        push_step(state, "push_start");

        // Process JPS
        while (!state.is_complete && 
               !state.target_found && 
               !state.open_set.empty())
        {
            auto [current_f, current_pos] = state.open_set.top();
            state.open_set.pop();

            // Skipping stale queue entries
            if (current_f > m_cells[current_pos.row][current_pos.col].f_score)
            {
                continue;
            }

            m_cells[current_pos.row][current_pos.col].in_open = false;
            state.current_pos = current_pos;

            state.phase = JPSState::Phase::POP_MIN;
            push_step(state, "pop_min");

            // Goal check
            if (current_pos == m_target)
            {
                state.target_found = true;
                reconstruct_path();
                state.phase = JPSState::Phase::TARGET_FOUND;
                push_step(state, "target_found");
                break;
            }

            // Add to closed set
            m_cells[current_pos.row][current_pos.col].visited = true;
            state.explored_count++;
            m_visited_order.push_back(current_pos);

            state.phase = JPSState::Phase::ADD_TO_CLOSED;
            push_step(state, "add_to_closed");

            // Find jump points from current node
            state.phase = JPSState::Phase::FIND_JUMP_POINTS;
            push_step(state, "find_jump_points");

            auto parent = m_cells[current_pos.row][current_pos.col].parent;
            auto jump_points = get_pruned_neighbors(current_pos, parent);

            state.phase = JPSState::Phase::PRUNE_NEIGHBORS;
            push_step(state, "prune_neighbors");

            for (const auto& jump_point : jump_points)
            {
                state.current_jump_point = jump_point;
                state.comparisons++;

                // Skip if in closed set
                if (m_cells[jump_point.row][jump_point.col].visited)
                    continue;

                // Mark as jump point
                if (!m_cells[jump_point.row][jump_point.col].is_jump_point)
                {
                    m_cells[jump_point.row][jump_point.col].is_jump_point = true;
                    state.jump_count++;
                    m_jump_points.push_back(jump_point);
                }

                state.phase = JPSState::Phase::EVALUATE_JUMP_POINT;
                push_step(state, "evaluate_jump_point");

                // Calculate distance to jump point
                int dx = std::abs(jump_point.col - current_pos.col);
                int dy = std::abs(jump_point.row - current_pos.row);
                float distance = (dx == dy) ? static_cast<float>(core::SQRT2) * dx :
                                 (dx == 0 || dy == 0) ? static_cast<float>(dx + dy) : 0;
                float tentative_g = m_cells[current_pos.row][current_pos.col].g_score + distance;

                state.tentative_g_score = tentative_g;
                state.old_g_score = m_cells[jump_point.row][jump_point.col].g_score;
                state.current_edge_weight = distance;

                state.phase = JPSState::Phase::RELAX_EDGE;
                push_step(state, "relax_edge");

                if (tentative_g < m_cells[jump_point.row][jump_point.col].g_score)
                {
                    m_cells[jump_point.row][jump_point.col].parent = current_pos;
                    m_cells[jump_point.row][jump_point.col].g_score = tentative_g;
                    float h_score = calculate_heuristic(jump_point, m_target);
                    float new_f = tentative_g + h_score;
                    m_cells[jump_point.row][jump_point.col].f_score = new_f;
                    state.new_f_score = new_f;
                    state.new_g_score = tentative_g;

                    state.phase = JPSState::Phase::UPDATE_SCORES;
                    push_step(state, "update_scores");

                    state.open_set.emplace(new_f, jump_point);
                    m_cells[jump_point.row][jump_point.col].in_open = true;

                    state.phase = JPSState::Phase::PUSH_NEIGHBOR;
                    push_step(state, "push_neighbor");
                }
            }

            state.current_jump_point = GridPosition(-1, -1);
        }

        if (state.open_set.empty() && !state.target_found)
        {
            state.is_complete = true;
            state.phase = JPSState::Phase::OPEN_SET_EMPTY;
            push_step(state, "open_set_empty");
        }

        state.is_complete = true;
        state.phase = JPSState::Phase::COMPLETED;
        push_step(state, "completed");

        LOG_DEBUG(
            "Generated {} steps for JPS with {} jump points", 
            m_steps.size(), 
            state.jump_count
        );
    }

    void JumpPointSearch::push_step(
        const JPSState& state, 
        const std::string& operation_id)
    {
        auto step = create_step_from_state(state, operation_id);
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep JumpPointSearch::create_step_from_state(
        const JPSState& state, 
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

    void JumpPointSearch::append_interpolated_path(
        std::vector<GridPosition>& path,
        const GridPosition& from,
        const GridPosition& to)
    {
        int dx = to.col - from.col;
        int dy = to.row - from.row;

        dx = (dx == 0) ? 0 : dx / std::abs(dx);
        dy = (dy == 0) ? 0 : dy / std::abs(dy);

        GridPosition current = from;

        while (current != to)
        {
            current.row += dy;
            current.col += dx;

            path.push_back(current);
        }
    }

    void JumpPointSearch::populate_step_metadata(
        AlgorithmStep& step,
        const JPSState& state,
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
            "Number of jump point evaluations"
        );
        step.metadata.set(
            "jump_points_found", 
            state.jump_count, 
            "Number of jump points identified"
        );
        step.metadata.set(
            "tentative_g", 
            state.tentative_g_score, 
            "Potential new g-score"
        );
        step.metadata.set(
            "old_g", 
            state.old_g_score, 
            "Current g-score"
        );
        step.metadata.set(
            "new_g", 
            state.new_g_score, 
            "Updated g-score"
        );
        step.metadata.set(
            "new_f", 
            state.new_f_score, 
            "Updated f-score"
        );

        // FIX: Check if current_pos is valid before accessing
        if (state.current_pos.row >= 0 && state.current_pos.row < m_rows &&
            state.current_pos.col >= 0 && state.current_pos.col < m_cols)
        {
            float g_score = m_cells[state.current_pos.row][state.current_pos.col].g_score;
            float f_score = m_cells[state.current_pos.row][state.current_pos.col].f_score;

            step.metadata.set(
                "g_score",
                std::isfinite(g_score) ? g_score : -1.0f,
                "Current g-score"
            );

            step.metadata.set(
                "f_score",
                std::isfinite(f_score) ? f_score : -1.0f,
                "Current f-score"
            );

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

            // Get parent information
            auto parent = m_cells[state.current_pos.row][state.current_pos.col].parent;
            if (parent.row >= 0 && parent.col >= 0)
            {
                step.metadata.set(
                    "parent_row", 
                    parent.row, 
                    "Parent row"
                );
                step.metadata.set(
                    "parent_col", 
                    parent.col, 
                    "Parent col"
                );
            }
        }
        else
        {
            // Set default values when no current node
            step.metadata.set(
                "g_score", 
                -1.0f, 
                "Current g-score (not available)"
            );
            step.metadata.set(
                "f_score", 
                -1.0f, 
                "Current f-score (not available)"
            );
        }

        // Handle jump point information (always check validity)
        if (state.current_jump_point.row >= 0 && state.current_jump_point.row < m_rows &&
            state.current_jump_point.col >= 0 && state.current_jump_point.col < m_cols)
        {
            step.metadata.set(
                "jump_point_row", 
                state.current_jump_point.row, 
                "Jump point row"
            );
            step.metadata.set(
                "jump_point_col", 
                state.current_jump_point.col, 
                "Jump point column"
            );
            step.metadata.set(
                "edge_weight", 
                state.current_edge_weight, 
                "Distance to jump point"
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
                    "Previous g-score"
                );
            }
            if (std::isfinite(state.new_f_score))
            {
                step.metadata.set(
                    "new_f", 
                    state.new_f_score, 
                    "New f-score for jump point"
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
            if (m_target.row >= 0 && m_target.row < m_rows &&
                m_target.col >= 0 && m_target.col < m_cols)
            {
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
        }

        // Phase as string
        std::string phase_str;
        switch (state.phase)
        {
            case JPSState::Phase::INITIALIZE: 
                phase_str = "Initializing"; break;
            case JPSState::Phase::PUSH_START: 
                phase_str = "Pushing Start Cell"; break;
            case JPSState::Phase::POP_MIN: 
                phase_str = "Popping Minimum F-Score"; break;
            case JPSState::Phase::GOAL_CHECK: 
                phase_str = "Goal Check"; break;
            case JPSState::Phase::ADD_TO_CLOSED: 
                phase_str = "Adding to Closed Set"; break;
            case JPSState::Phase::FIND_JUMP_POINTS: 
                phase_str = "Finding Jump Points"; break;
            case JPSState::Phase::PRUNE_NEIGHBORS: 
                phase_str = "Pruning Neighbors"; break;
            case JPSState::Phase::EVALUATE_JUMP_POINT: 
                phase_str = "Evaluating Jump Point"; break;
            case JPSState::Phase::RELAX_EDGE: 
                phase_str = "Relaxing Edge"; break;
            case JPSState::Phase::UPDATE_SCORES: 
                phase_str = "Updating Scores"; break;
            case JPSState::Phase::PUSH_NEIGHBOR: 
                phase_str = "Pushing to Open Set"; break;
            case JPSState::Phase::TARGET_FOUND: 
                phase_str = "Target Found!"; break;
            case JPSState::Phase::OPEN_SET_EMPTY: 
                phase_str = "Open Set Empty - No Path"; break;
            case JPSState::Phase::COMPLETED: 
                phase_str = "Completed"; break;
        }
        step.metadata.set(
            "phase", 
            phase_str, 
            "Current JPS phase"
        );
    }

    void JumpPointSearch::update_visualization_data(
        AlgorithmStep& step, 
        const JPSState& state, 
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
                }
            }
        }

        // Mark frontier cells
        for (int row = 0; row < m_rows; ++row)
        {
            for (int col = 0; col < m_cols; ++col)
            {
                if (m_cells[row][col].in_open)
                {
                    viz.graph_state.frontier_nodes.push_back(
                        to_node_id(row, col)
                    );
                }
            }
        }

        // Mark jump points specially (only if valid)
        for (const auto& jp : m_jump_points)
        {
            if (jp.row >= 0 && jp.row < m_rows && jp.col >= 0 && jp.col < m_cols)
            {
                viz.graph_state.node_distances[to_node_id(jp.row, jp.col)] = 1000;
            }
        }

        // Mark current node (only if valid)
        if (state.current_pos.row >= 0 && state.current_pos.row < m_rows &&
            state.current_pos.col >= 0 && state.current_pos.col < m_cols)
        {
            viz.highlighted_index = to_node_id(
                state.current_pos.row, state.current_pos.col
            );
        }

        // Mark current jump point being evaluated (only if both valid)
        if (state.current_jump_point.row >= 0 && state.current_jump_point.row < m_rows &&
            state.current_jump_point.col >= 0 && state.current_jump_point.col < m_cols &&
            state.current_pos.row >= 0 && state.current_pos.row < m_rows &&
            state.current_pos.col >= 0 && state.current_pos.col < m_cols)
        {
            viz.graph_state.active_edges.emplace_back(
                to_node_id(state.current_pos.row, state.current_pos.col),
                to_node_id(state.current_jump_point.row, state.current_jump_point.col)
            );
            viz.additional_highlights.push_back(
                to_node_id(state.current_jump_point.row, state.current_jump_point.col)
            );
        }

        // Build path
        for (const auto& pos : m_path)
        {
            if (pos.row >= 0 && pos.row < m_rows && 
                pos.col >= 0 && pos.col < m_cols)
            {
                viz.graph_state.path.push_back(
                    to_node_id(pos.row, pos.col)
                );
            }
        }

        viz.comparison_count = state.comparisons;
        viz.is_complete = state.is_complete || state.target_found;
    }

    void JumpPointSearch::reset_state()
    {
        m_current_state = JPSState{};
        m_path.clear();
        m_visited_order.clear();
        m_jump_points.clear();

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