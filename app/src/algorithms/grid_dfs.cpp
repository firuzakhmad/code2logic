//
// Created by Akhmad on 5/20/26.
//

#include "algorithms/grid_dfs.hpp"
#include "core/utils/logger/logger.hpp"

#include <algorithm>
#include <stack>

namespace c2l::algorithms
{
    GridDFS::GridDFS(core::JsonConfigManager& json_config_manager)
        : GridPathfindingBase(json_config_manager, AlgorithmType::GRID_DFS)
    {
        m_grid.assign(20, std::vector<GridCellType>(20, GridCellType::EMPTY));
        LOG_DEBUG("GridDFS created with default 20x20 grid");
    }

    std::vector<GridPosition> GridDFS::get_neighbors(
        const GridPosition& pos
    ) const
    {
        std::vector<GridPosition> neighbors;

        // Cardinal directions first (4-directional)
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
                    // Checking if diagonal movement is allowed (not cutting corners)
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

    void GridDFS::generate_all_steps()
    {
        if (m_rows == 0 || m_cols == 0)
        {
            LOG_WARNING(
                "Cannot generate GridDFS steps: grid not initialized (rows={}, cols={})", 
                m_rows, 
                m_cols
            );
            GridDFSState empty_state;
            empty_state.is_complete = true;
            empty_state.phase = GridDFSState::Phase::COMPLETED;
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
            GridDFSState empty_state;
            empty_state.is_complete = true;
            empty_state.phase = GridDFSState::Phase::COMPLETED;
            push_step(empty_state, "error_invalid_positions");
            return;
        }

        m_steps.clear();
        m_visited_order.clear();
        m_frontier_order.clear();

        // Initializing cells
        m_cells.assign(m_rows, std::vector<GridCell>(m_cols));
        for (int row = 0; row < m_rows; ++row)
        {
            for (int col = 0; col < m_cols; ++col)
            {
                m_cells[row][col] = GridCell(row, col);
            }
        }

        // Initializing state
        GridDFSState state;
        state.phase = GridDFSState::Phase::INITIALIZE;
        state.visited_count = 0;
        state.comparisons = 0;
        state.time_counter = 0;
        state.is_complete = false;
        state.target_found = false;

        push_step(state, "init");

        // Pushing start node to stack
        state.stack.push(m_start);
        m_cells[m_start.row][m_start.col].in_stack = true;
        m_cells[m_start.row][m_start.col].discovery_time = state.time_counter++;
        m_frontier_order.push_back(m_start);

        state.phase = GridDFSState::Phase::PUSH_START;
        push_step(state, "push_start");

        // Processing GridDFS until complete
        while (!state.is_complete && !state.target_found && !state.stack.empty())
        {
            // Popping top cell from stack
            GridPosition current_pos = state.stack.top();
            state.stack.pop();
            state.current_pos = current_pos;
            state.current_depth = m_cells[current_pos.row][current_pos.col].discovery_time;

            // Mark as visited
            if (!m_cells[current_pos.row][current_pos.col].visited)
            {
                m_cells[current_pos.row][current_pos.col].visited = true;
                m_cells[current_pos.row][current_pos.col].in_stack = false;
                state.visited_count++;
                m_visited_order.push_back(current_pos);
            }

            state.phase = GridDFSState::Phase::POP_TOP;
            push_step(state, "pop_top");

            state.phase = GridDFSState::Phase::ADD_TO_VISITED;
            push_step(state, "add_to_visited");

            // Goal check
            state.phase = GridDFSState::Phase::GOAL_CHECK;
            push_step(state, "goal_check");

            if (current_pos == m_target)
            {
                state.target_found = true;
                reconstruct_path();
                state.phase = GridDFSState::Phase::TARGET_FOUND;
                push_step(state, "target_found");
                break;
            }

            // Get neighbors (reverse order for consistent visualization)
            auto neighbors = get_neighbors(current_pos);
            // Reverse to maintain order (stack is LIFO)
            std::reverse(neighbors.begin(), neighbors.end());

            bool found_new_neighbor = false;

            for (const auto& neighbor : neighbors)
            {
                state.current_neighbor = neighbor;
                state.comparisons++;

                state.phase = GridDFSState::Phase::EVALUATE_NEIGHBOR;
                push_step(state, "evaluate_neighbor");

                // Skip if already visited or in stack
                if (m_cells[neighbor.row][neighbor.col].visited ||
                    m_cells[neighbor.row][neighbor.col].in_stack)
                {
                    continue;
                }

                found_new_neighbor = true;

                state.phase = GridDFSState::Phase::DISCOVER_NEIGHBOR;
                push_step(state, "discover_neighbor");

                // Discover new node
                m_cells[neighbor.row][neighbor.col].parent = current_pos;
                m_cells[neighbor.row][neighbor.col].discovery_time = state.time_counter++;
                m_cells[neighbor.row][neighbor.col].in_stack = true;
                state.stack.push(neighbor);
                m_frontier_order.push_back(neighbor);

                state.phase = GridDFSState::Phase::PUSH_NEIGHBOR;
                push_step(state, "push_neighbor");
            }

            if (!found_new_neighbor)
            {
                // Backtrack - no more neighbors to explore
                m_cells[current_pos.row][current_pos.col].finish_time = state.time_counter++;
                state.phase = GridDFSState::Phase::BACKTRACK;
                push_step(state, "backtrack");
            }

            state.current_neighbor = GridPosition(-1, -1);
        }

        // Final steps
        if (state.stack.empty() && !state.target_found)
        {
            state.is_complete = true;
            state.phase = GridDFSState::Phase::STACK_EMPTY;
            push_step(state, "stack_empty");
        }

        if (state.target_found)
        {
            reconstruct_path();
        }

        state.is_complete = true;
        state.phase = GridDFSState::Phase::COMPLETED;
        push_step(state, "completed");

        LOG_DEBUG("Generated {} steps for GridDFS", m_steps.size());
    }

    void GridDFS::push_step(
        const GridDFSState& state, 
        const std::string& operation_id)
    {
        auto step = create_step_from_state(state, operation_id);
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep GridDFS::create_step_from_state(
        const GridDFSState& state, 
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
        populate_step_metadata(step, state, operation_id);
        step.description = format_step_description(operation_id, step);
        update_visualization_data(step, state, operation_id);

        return step;
    }

    void GridDFS::populate_step_metadata(
        AlgorithmStep& step, 
        const GridDFSState& state, 
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
            "stack_size", 
            state.stack.size(), 
            "Current stack size"
        );
        step.metadata.set(
            "visited_count", 
            state.visited_count,
            "Number of cells visited"
        );
        step.metadata.set(
            "comparisons", 
            state.comparisons, 
            "Number of neighbor evaluations"
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
            step.metadata.set(
                "current_depth", 
                state.current_depth, 
                "Discovery time of current node"
            );
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
        }

        if (state.target_found)
        {
            step.metadata.set(
                "path_length", 
                m_path.size(), 
                "Length of found path"
            );
        }

        // Phase as string
        std::string phase_str;
        switch (state.phase)
        {
            case GridDFSState::Phase::INITIALIZE: 
                phase_str = "Initializing"; break;
            case GridDFSState::Phase::PUSH_START: 
                phase_str = "Pushing Start Cell"; break;
            case GridDFSState::Phase::POP_TOP: 
                phase_str = "Popping Top of Stack"; break;
            case GridDFSState::Phase::GOAL_CHECK: 
                phase_str = "Goal Check"; break;
            case GridDFSState::Phase::ADD_TO_VISITED: 
                phase_str = "Adding to Visited Set"; break;
            case GridDFSState::Phase::EVALUATE_NEIGHBOR: 
                phase_str = "Evaluating Neighbor"; break;
            case GridDFSState::Phase::DISCOVER_NEIGHBOR: 
                phase_str = "Discovering Neighbor"; break;
            case GridDFSState::Phase::PUSH_NEIGHBOR: 
                phase_str = "Pushing to Stack"; break;
            case GridDFSState::Phase::BACKTRACK: 
                phase_str = "Backtracking"; break;
            case GridDFSState::Phase::TARGET_FOUND: 
                phase_str = "Target Found!"; break;
            case GridDFSState::Phase::STACK_EMPTY: 
                phase_str = "Stack Empty - No Path"; break;
            case GridDFSState::Phase::COMPLETED: 
                phase_str = "Completed"; break;
        }
        step.metadata.set(
            "phase", 
            phase_str, 
            "Current GridDFS phase"
        );
    }

    void GridDFS::update_visualization_data(
        AlgorithmStep& step, 
        const GridDFSState& state, 
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
                    if (m_cells[row][col].discovery_time >= 0)
                    {
                        viz.graph_state.node_distances[to_node_id(row, col)] =
                            m_cells[row][col].discovery_time;
                    }
                }
            }
        }

        // Marking frontier cells (in stack)
        for (int row = 0; row < m_rows; ++row)
        {
            for (int col = 0; col < m_cols; ++col)
            {
                if (m_cells[row][col].in_stack && !m_cells[row][col].visited)
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

        viz.comparison_count = state.comparisons;
        viz.visited_node_count = state.visited_count;
        viz.is_complete = state.is_complete || state.target_found;
    }

    void GridDFS::reset_state()
    {
        m_current_state = GridDFSState{};
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