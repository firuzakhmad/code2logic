#include "algorithms/best_first_search.hpp"
#include "core/utils/logger/logger.hpp"

#include <algorithm>
#include <cmath>
#include <queue>

#include "core/utils/variables.hpp"

namespace c2l::algorithms
{
    BestFirstSearch::BestFirstSearch(core::JsonConfigManager& json_config_manager)
        : GridPathfindingBase(json_config_manager, AlgorithmType::BEST_FIRST_SEARCH)
    {
        LOG_DEBUG("Best-First Search created with default 20x20 grid");
    }

    float BestFirstSearch::get_movement_cost(
        const GridPosition& from, 
        const GridPosition& to
    ) const
    {
        // Best-First doesn't use movement cost for selection, but we keep for completeness
        bool is_diagonal = (from.row != to.row && from.col != to.col);
        return (is_diagonal && m_allow_diagonals) ? core::SQRT2 : 1.0f;
    }

    std::vector<GridPosition> BestFirstSearch::get_neighbors(
        const GridPosition& pos
    ) const
    {
        std::vector<GridPosition> neighbors;

        // Cardinal directions
        const int dr4[] = {-1, 0, 1, 0};
        const int dc4[] = {0, 1, 0, -1};

        for (int i = 0; i < 4; ++i)
        {
            int nr = pos.row + dr4[i];
            int nc = pos.col + dc4[i];

            if (is_walkable(nr, nc))
            {
                neighbors.emplace_back(nr, nc);
            }
        }

        // Diagonal directions (if allowed)
        if (m_allow_diagonals)
        {
            const int dr8[] = {-1, -1, 1, 1};
            const int dc8[] = {-1, 1, -1, 1};

            for (int i = 0; i < 4; ++i)
            {
                int nr = pos.row + dr8[i];
                int nc = pos.col + dc8[i];

                if (is_walkable(nr, nc))
                {
                    // Check if diagonal movement is allowed (not cutting corners)
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
            }
        }

        return neighbors;
    }

    float BestFirstSearch::calculate_heuristic(
        const GridPosition& pos, 
        const GridPosition& target
    ) const
    {
        float dx = static_cast<float>(pos.col - target.col);
        float dy = static_cast<float>(pos.row - target.row);

        switch (m_heuristic_type)  // This comes from GridPathfindingBase
        {
            case HeuristicType::Euclidean:
                return std::sqrt(dx * dx + dy * dy);
            case HeuristicType::Manhattan:
                return std::abs(dx) + std::abs(dy);
            case HeuristicType::Chebyshev:
                return std::max(std::abs(dx), std::abs(dy));
            case HeuristicType::Octile:
                return std::max(
                    std::abs(dx), std::abs(dy)) + (core::SQRT2 - 1.0f) * std::min(std::abs(dx), 
                    std::abs(dy)
                );
            default:
                return std::sqrt(dx * dx + dy * dy);
        }
    }

    void BestFirstSearch::generate_all_steps()
    {
        if (m_rows == 0 || m_cols == 0)
        {
            LOG_WARNING(
                "Cannot generate Best-First Search steps: grid not initialized"
            );
            BestFirstState empty_state;
            empty_state.is_complete = true;
            empty_state.phase = BestFirstState::Phase::COMPLETED;
            push_step(empty_state, "error_no_grid");
            return;
        }

        if (m_start.row < 0 || m_start.col < 0 || 
            m_target.row < 0 || m_target.col < 0)
        {
            LOG_ERROR("Invalid start or target position");
            BestFirstState empty_state;
            empty_state.is_complete = true;
            empty_state.phase = BestFirstState::Phase::COMPLETED;
            push_step(empty_state, "error_invalid_positions");
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
            }
        }

        // Initialization state
        BestFirstState state;
        state.phase = BestFirstState::Phase::INITIALIZE;
        state.explored_count = 0;
        state.comparisons = 0;
        state.is_complete = false;
        state.target_found = false;

        push_step(state, "init");

        // Calculating heuristic for start and push to open set
        float start_h = calculate_heuristic(m_start, m_target);
        m_cells[m_start.row][m_start.col].h_score = start_h;
        m_cells[m_start.row][m_start.col].f_score = start_h;
        state.open_set.emplace(start_h, m_start);
        m_cells[m_start.row][m_start.col].in_open = true;
        m_frontier_order.push_back(m_start);

        state.phase = BestFirstState::Phase::PUSH_START;
        push_step(state, "push_start");

        // Processing Best-First Search until complete
        while (!state.is_complete && 
               !state.target_found && 
               !state.open_set.empty())
        {
            // Popping node with minimum heuristic value
            auto [current_h, current_pos] = state.open_set.top();
            state.open_set.pop();
            m_cells[current_pos.row][current_pos.col].in_open = false;
            state.current_pos = current_pos;
            state.current_heuristic = current_h;

            state.phase = BestFirstState::Phase::POP_MIN;
            push_step(state, "pop_min");

            // Marking as visited (processed)
            if (!m_cells[current_pos.row][current_pos.col].visited)
            {
                m_cells[current_pos.row][current_pos.col].visited = true;
                state.explored_count++;
                m_visited_order.push_back(current_pos);
            }

            state.phase = BestFirstState::Phase::ADD_TO_VISITED;
            push_step(state, "add_to_visited");

            // Goal check
            state.phase = BestFirstState::Phase::GOAL_CHECK;
            push_step(state, "goal_check");

            if (current_pos == m_target)
            {
                state.target_found = true;
                reconstruct_path();
                state.phase = BestFirstState::Phase::TARGET_FOUND;
                push_step(state, "target_found");
                break;
            }

            // Evaluate all neighbors
            auto neighbors = get_neighbors(current_pos);

            for (const auto& neighbor : neighbors)
            {
                state.current_neighbor = neighbor;
                state.comparisons++;

                state.phase = BestFirstState::Phase::EVALUATE_NEIGHBOR;
                push_step(state, "evaluate_neighbor");

                // Skip if already visited
                if (m_cells[neighbor.row][neighbor.col].visited)
                    continue;

                state.phase = BestFirstState::Phase::COMPUTE_HEURISTIC;
                push_step(state, "compute_heuristic");

                // Calculate heuristic for neighbor
                float neighbor_h = calculate_heuristic(neighbor, m_target);
                state.new_h_score = neighbor_h;

                state.phase = BestFirstState::Phase::UPDATE_SCORES;
                push_step(state, "update_scores");

                // Best-First only uses heuristic for priority
                // But we still track parent for path reconstruction
                if (!m_cells[neighbor.row][neighbor.col].in_open)
                {
                    m_cells[neighbor.row][neighbor.col].parent = current_pos;
                    m_cells[neighbor.row][neighbor.col].h_score = neighbor_h;
                    m_cells[neighbor.row][neighbor.col].f_score = neighbor_h;
                    state.open_set.emplace(neighbor_h, neighbor);
                    m_cells[neighbor.row][neighbor.col].in_open = true;
                    m_frontier_order.push_back(neighbor);

                    state.phase = BestFirstState::Phase::PUSH_NEIGHBOR;
                    push_step(state, "push_neighbor");
                }
                else
                {
                    // If already in open set, we might want to update parent if this path is better
                    // But for pure Best-First, we don't need to
                    state.phase = BestFirstState::Phase::PUSH_NEIGHBOR;
                    push_step(state, "push_neighbor_skip");
                }
            }

            state.current_neighbor = GridPosition(-1, -1);
        }

        // Final steps
        if (state.open_set.empty() && !state.target_found)
        {
            state.is_complete = true;
            state.phase = BestFirstState::Phase::OPEN_SET_EMPTY;
            push_step(state, "open_set_empty");
        }

        if (state.target_found)
        {
            reconstruct_path();
        }

        state.is_complete = true;
        state.phase = BestFirstState::Phase::COMPLETED;
        push_step(state, "completed");

        LOG_DEBUG("Generated {} steps for Best-First Search", m_steps.size());
    }

    void BestFirstSearch::push_step(
        const BestFirstState& state, 
        const std::string& operation_id)
    {
        auto step = create_step_from_state(state, operation_id);
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep BestFirstSearch::create_step_from_state(
        const BestFirstState& state, 
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

    void BestFirstSearch::populate_step_metadata(
        AlgorithmStep& step, 
        const BestFirstState& state, 
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
            "visited_count", 
            state.explored_count, 
            "Number of cells visited"
        );
        step.metadata.set(
            "comparisons", 
            state.comparisons, 
            "Number of neighbor evaluations"
        );
        step.metadata.set(
            "heuristic_type", 
            heuristic_type_to_string(m_heuristic_type), 
            "Heuristic function used"
        );
        step.metadata.set(
            "tie_breaking", 
            m_use_tie_breaking ? "Enabled" : "Disabled", 
            "Tie-breaking strategy"
        );

        if (std::isfinite(m_cells[m_start.row][m_start.col].h_score))
        {
            step.metadata.set(
                "heuristic", m_cells[m_start.row][m_start.col].h_score, 
                "Heuristic value"
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
            step.metadata.set(
                "current_heuristic", 
                state.current_heuristic, 
                "Heuristic value of current cell"
            );

            if (std::isfinite(m_cells[state.current_pos.row][state.current_pos.col].h_score))
            {
                step.metadata.set(
                    "h_score", 
                    m_cells[state.current_pos.row][state.current_pos.col].h_score, 
                    "Heuristic score"
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
                "neighbor_heuristic", 
                state.new_h_score, 
                "Heuristic value of neighbor"
            );
        }

        if (state.target_found)
        {
            step.metadata.set(
                "path_length", 
                m_path.size(), 
                "Length of found path"
            );

            // Calculating approximate path cost (sum of Euclidean distances)
            float path_cost = 0.0f;
            for (size_t i = 1; i < m_path.size(); ++i)
            {
                float dx = static_cast<float>(m_path[i].col - m_path[i-1].col);
                float dy = static_cast<float>(m_path[i].row - m_path[i-1].row);
                path_cost += std::sqrt(dx * dx + dy * dy);
            }
            step.metadata.set(
                "path_cost", 
                path_cost, 
                "Approximate path length"
            );
        }

        // Phase as string
        std::string phase_str;
        switch (state.phase)
        {
            case BestFirstState::Phase::INITIALIZE: 
                phase_str = "Initializing"; break;
            case BestFirstState::Phase::PUSH_START: 
                phase_str = "Pushing Start Cell"; break;
            case BestFirstState::Phase::POP_MIN: 
                phase_str = "Popping Minimum Heuristic"; break;
            case BestFirstState::Phase::GOAL_CHECK: 
                phase_str = "Goal Check"; break;
            case BestFirstState::Phase::ADD_TO_VISITED: 
                phase_str = "Adding to Visited Set"; break;
            case BestFirstState::Phase::EVALUATE_NEIGHBOR: 
                phase_str = "Evaluating Neighbor"; break;
            case BestFirstState::Phase::COMPUTE_HEURISTIC: 
                phase_str = "Computing Heuristic"; break;
            case BestFirstState::Phase::UPDATE_SCORES: 
                phase_str = "Updating Scores"; break;
            case BestFirstState::Phase::PUSH_NEIGHBOR: 
                phase_str = "Pushing to Open Set"; break;
            case BestFirstState::Phase::TARGET_FOUND: 
                phase_str = "Target Found!"; break;
            case BestFirstState::Phase::OPEN_SET_EMPTY: 
                phase_str = "Open Set Empty - No Path"; break;
            case BestFirstState::Phase::COMPLETED: 
                phase_str = "Completed"; break;
        }
        step.metadata.set(
            "phase", 
            phase_str, 
            "Current Best-First Search phase"
        );
    }

    void BestFirstSearch::update_visualization_data(
        AlgorithmStep& step, 
        const BestFirstState& state, 
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

        // Marking visited cells
        for (int row = 0; row < m_rows; ++row)
        {
            for (int col = 0; col < m_cols; ++col)
            {
                if (m_cells[row][col].visited)
                {
                    viz.graph_state.visited_nodes.push_back(
                        to_node_id(row, col)
                    );
                    // Storing heuristic value as distance for visualization
                    if (std::isfinite(m_cells[row][col].h_score))
                    {
                        viz.graph_state.node_distances[to_node_id(row, col)] =
                            static_cast<int>(m_cells[row][col].h_score * 10.0f);
                    }
                }
            }
        }

        // Marking frontier cells (open set)
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

        // Building path
        for (const auto& pos : m_path)
        {
            viz.graph_state.path.push_back(
                to_node_id(pos.row, pos.col)
            );
        }

        // Marking active edge (current neighbor being evaluated)
        if (state.current_neighbor.row >= 0 && state.current_pos.row >= 0)
        {
            viz.graph_state.active_edges.emplace_back(
                to_node_id(state.current_pos.row, state.current_pos.col),
                to_node_id(state.current_neighbor.row, state.current_neighbor.col)
            );
            viz.additional_highlights.push_back(
                to_node_id(state.current_neighbor.row, state.current_neighbor.col)
            );
        }

        // Visualizing heuristic heatmap (optional - can be enabled in visualizer)
        for (int row = 0; row < m_rows; ++row)
        {
            for (int col = 0; col < m_cols; ++col)
            {
                if (!m_cells[row][col].visited && 
                    !m_cells[row][col].in_open && 
                    is_walkable(row, col))
                {
                    float h = calculate_heuristic(GridPosition(row, col), m_target);
                    if (std::isfinite(h))
                    {
                        // Useing node_distances to store heuristic values for heatmap
                        if (viz.graph_state.node_distances.find(
                                to_node_id(row, col)) == viz.graph_state.node_distances.end()
                            )
                        {
                            viz.graph_state.node_distances[to_node_id(row, col)] = static_cast<int>(h * 10.0f);
                        }
                    }
                }
            }
        }

        viz.comparison_count = state.comparisons;
        viz.is_complete = state.is_complete || state.target_found;
    }

    void BestFirstSearch::reset_state()
    {
        m_current_state = BestFirstState{};
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
                }
            }
        }
    }

} // namespace c2l::algorithms