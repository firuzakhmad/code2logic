//
// Created by Akhmad on 5/20/26.
//

#include "algorithms/grid_dijkstra.hpp"
#include "core/utils/logger/logger.hpp"
#include "core/utils/variables.hpp"


#include <algorithm>
#include <cmath>
#include <queue>
#include <limits>

namespace c2l::algorithms
{
    GridDijkstra::GridDijkstra(core::JsonConfigManager& json_config_manager)
        : GridPathfindingBase(json_config_manager, AlgorithmType::GRID_DIJKSTRA)
    {
        LOG_DEBUG("GridDijkstra created with default 20x20 grid");
    }

    float GridDijkstra::get_movement_cost(
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
        if (weight >= core::INF) return core::INF;
        cost *= static_cast<float>(weight);

        return cost;
    }

    std::vector<GridPosition> GridDijkstra::get_neighbors(
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

            if (is_walkable(nr, nc))
            {
                neighbors.emplace_back(nr, nc);
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

    void GridDijkstra::generate_all_steps()
    {
        if (m_rows == 0 || m_cols == 0)
        {
            LOG_WARNING(
                "Cannot generate GridDijkstra steps: grid not initialized (rows={}, cols={})", 
                m_rows, 
                m_cols
            );
            GridDijkstraState empty_state;
            empty_state.is_complete = true;
            empty_state.phase = GridDijkstraState::Phase::COMPLETED;
            push_step(empty_state, "error_no_grid");
            return;
        }

        if (m_start.row < 0 || m_start.col < 0 || 
            m_target.row < 0 || m_target.col < 0)
        {
            LOG_ERROR(
                "Invalid start or target position: start=({},{}), target=({},{})",
                m_start.row, 
                m_start.col, 
                m_target.row, 
                m_target.col
            );
            GridDijkstraState empty_state;
            empty_state.is_complete = true;
            empty_state.phase = GridDijkstraState::Phase::COMPLETED;
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
                m_cells[row][col].weight = get_cell_weight(
                    GridPosition(row, col)
                );
            }
        }

        // Initialize state
        GridDijkstraState state;
        state.phase = GridDijkstraState::Phase::INITIALIZE;
        state.explored_count = 0;
        state.comparisons = 0;
        state.is_complete = false;
        state.target_found = false;

        push_step(state, "init");

        // Push start node to open set
        m_cells[m_start.row][m_start.col].distance = 0.0f;
        state.open_set.emplace(0.0f, m_start);
        m_cells[m_start.row][m_start.col].in_open = true;
        m_frontier_order.push_back(m_start);

        state.phase = GridDijkstraState::Phase::PUSH_START;
        push_step(state, "push_start");

        // Process GridDijkstra until complete
        while (!state.is_complete && 
               !state.target_found && 
               !state.open_set.empty())
        {
            // Pop node with minimum distance
            auto [current_dist, current_pos] = state.open_set.top();
            state.open_set.pop();

            // Skipping if we already processed this node (lazy deletion)
            if (m_cells[current_pos.row][current_pos.col].visited)
                continue;

            m_cells[current_pos.row][current_pos.col].in_open = false;
            state.current_pos = current_pos;

            state.phase = GridDijkstraState::Phase::POP_MIN;
            push_step(state, "pop_min");

            // Goal check (if stopping at target)
            if (m_stop_at_target && current_pos == m_target)
            {
                state.target_found = true;
                reconstruct_path();
                state.phase = GridDijkstraState::Phase::TARGET_FOUND;
                push_step(state, "target_found");
                break;
            }

            // Add to closed set
            m_cells[current_pos.row][current_pos.col].visited = true;
            state.explored_count++;
            m_visited_order.push_back(current_pos);

            state.phase = GridDijkstraState::Phase::ADD_TO_CLOSED;
            push_step(state, "add_to_closed");

            // Evaluate all neighbors
            auto neighbors = get_neighbors(current_pos);

            for (const auto& neighbor : neighbors)
            {
                state.current_neighbor = neighbor;
                state.comparisons++;

                state.phase = GridDijkstraState::Phase::EVALUATE_NEIGHBOR;
                push_step(state, "evaluate_neighbor");

                // Skip if already processed
                if (m_cells[neighbor.row][neighbor.col].visited)
                    continue;

                float move_cost = get_movement_cost(current_pos, neighbor);
                if (move_cost >= core::INF) continue;  // Wall or invalid

                float tentative_dist = m_cells[current_pos.row][current_pos.col].distance + move_cost;
                state.tentative_distance = tentative_dist;
                state.old_distance = m_cells[neighbor.row][neighbor.col].distance;
                state.current_edge_weight = move_cost;

                state.phase = GridDijkstraState::Phase::RELAX_EDGE;
                push_step(state, "relax_edge");

                if (tentative_dist < m_cells[neighbor.row][neighbor.col].distance)
                {
                    m_cells[neighbor.row][neighbor.col].parent = current_pos;
                    m_cells[neighbor.row][neighbor.col].distance = tentative_dist;

                    state.phase = GridDijkstraState::Phase::UPDATE_DISTANCE;
                    push_step(state, "update_distance");

                    if (!m_cells[neighbor.row][neighbor.col].in_open)
                    {
                        state.open_set.emplace(tentative_dist, neighbor);
                        m_cells[neighbor.row][neighbor.col].in_open = true;
                        m_frontier_order.push_back(neighbor);

                        state.phase = GridDijkstraState::Phase::PUSH_NEIGHBOR;
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
            state.phase = GridDijkstraState::Phase::OPEN_SET_EMPTY;
            push_step(state, "open_set_empty");
        }

        // Reconstruct path if target found (or always if we want the shortest path to all nodes)
        if (state.target_found)
        {
            reconstruct_path();
        }

        state.is_complete = true;
        state.phase = GridDijkstraState::Phase::COMPLETED;
        push_step(state, "completed");

        LOG_DEBUG("Generated {} steps for GridDijkstra", m_steps.size());
    }

    void GridDijkstra::push_step(
        const GridDijkstraState& state, 
        const std::string& operation_id)
    {
        auto step = create_step_from_state(state, operation_id);
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep GridDijkstra::create_step_from_state(
        const GridDijkstraState& state, 
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

    void GridDijkstra::populate_step_metadata(
        AlgorithmStep& step, 
        const GridDijkstraState& state, 
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

        // Defaults to avoid missing-variable errors
        step.metadata.set(
            "old_distance", 
            "N/A", 
            "Current distance of neighbor"
        );
        step.metadata.set(
            "new_distance", 
            "N/A", 
            "New shortest distance"
        );
        step.metadata.set(
            "tentative_distance", 
            "N/A", 
            "Potential new distance"
        );
        step.metadata.set(
            "current_distance", 
            "N/A", 
            "Current shortest distance"
        );

        if (state.current_pos.row >= 0)
        {
            step.metadata.set(
                "current_row", 
                state.current_pos.row, 
                "Current row");
            step.metadata.set(
                "current_col", 
                state.current_pos.col, 
                "Current column");

            if (std::isfinite(m_cells[state.current_pos.row][state.current_pos.col].distance))
            {
                step.metadata.set(
                    "current_distance",
                    m_cells[state.current_pos.row][state.current_pos.col].distance,
                    "Current shortest distance from start");
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

            if (std::isfinite(state.tentative_distance))
            {
                step.metadata.set(
                    "tentative_distance", 
                    state.tentative_distance, 
                    "Potential new distance"
                );
            }
            if (std::isfinite(state.old_distance))
            {
                step.metadata.set(
                    "old_distance", 
                    state.old_distance, 
                    "Current distance of neighbor"
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

            float path_cost = m_cells[m_target.row][m_target.col].distance;
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
            case GridDijkstraState::Phase::INITIALIZE: 
                phase_str = "Initializing"; break;
            case GridDijkstraState::Phase::PUSH_START: 
                phase_str = "Pushing Start Cell"; break;
            case GridDijkstraState::Phase::POP_MIN: 
                phase_str = "Popping Minimum Distance"; break;
            case GridDijkstraState::Phase::GOAL_CHECK: 
                phase_str = "Goal Check"; break;
            case GridDijkstraState::Phase::ADD_TO_CLOSED: 
                phase_str = "Adding to Closed Set"; break;
            case GridDijkstraState::Phase::EVALUATE_NEIGHBOR: 
                phase_str = "Evaluating Neighbor"; break;
            case GridDijkstraState::Phase::RELAX_EDGE: 
                phase_str = "Relaxing Edge"; break;
            case GridDijkstraState::Phase::UPDATE_DISTANCE: 
                phase_str = "Updating Distance"; break;
            case GridDijkstraState::Phase::PUSH_NEIGHBOR: 
                phase_str = "Pushing to Open Set"; break;
            case GridDijkstraState::Phase::TARGET_FOUND: 
                phase_str = "Target Found!"; break;
            case GridDijkstraState::Phase::OPEN_SET_EMPTY: 
                phase_str = "Open Set Empty - No Path"; break;
            case GridDijkstraState::Phase::COMPLETED: 
                phase_str = "Completed"; break;
        }
        step.metadata.set(
            "phase", 
            phase_str, 
            "Current GridDijkstra phase"
        );
    }

    void GridDijkstra::update_visualization_data(
        AlgorithmStep& step, 
        const GridDijkstraState& state, 
        const std::string& operation_id
    ) const
    {
        auto& viz = step.visualization;

        // For grid-based visualization
        viz.graph_state.visited_nodes.clear();
        viz.graph_state.frontier_nodes.clear();
        viz.graph_state.path.clear();
        viz.graph_state.node_distances.clear();

        // Convert 2D grid positions to 1D node IDs
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
                    viz.graph_state.visited_nodes.push_back(to_node_id(row, col));
                    if (m_cells[row][col].distance < core::INF)
                    {
                        viz.graph_state.node_distances[to_node_id(row, col)] =
                            static_cast<int>(m_cells[row][col].distance);
                    }
                }
            }
        }

        // Mark frontier cells (open set)
        for (int row = 0; row < m_rows; ++row)
        {
            for (int col = 0; col < m_cols; ++col)
            {
                if (m_cells[row][col].in_open && !m_cells[row][col].visited)
                {
                    viz.graph_state.frontier_nodes.push_back(to_node_id(row, col));
                }
            }
        }

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
        viz.visited_node_count = viz.graph_state.visited_nodes.size();
        viz.is_complete = state.is_complete || state.target_found;
    }

    void GridDijkstra::reset_state()
    {
        m_current_state = GridDijkstraState{};
        m_path.clear();
        m_visited_order.clear();
        m_frontier_order.clear();

        if (m_rows > 0 && m_cols > 0 && !m_grid.empty())
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