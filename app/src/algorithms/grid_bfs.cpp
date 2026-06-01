#include "algorithms/grid_bfs.hpp"
#include "core/utils/logger/logger.hpp"

#include <algorithm>
#include <queue>

namespace c2l::algorithms
{
    GridBFS::GridBFS(core::JsonConfigManager& json_config_manager)
        : GridPathfindingBase(json_config_manager, AlgorithmType::GRID_BFS)
    {
        m_grid.assign(20, std::vector<GridCellType>(20, GridCellType::EMPTY));
        LOG_DEBUG("GridBFS created with default 20x20 grid");
    }

    std::vector<GridPosition> GridBFS::get_neighbors(const GridPosition& pos) const
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

    void GridBFS::generate_all_steps()
    {
        if (m_rows == 0 || m_cols == 0)
        {
            LOG_WARNING(
                "Cannot generate GridBFS steps: grid not initialized (rows={}, cols={})", 
                m_rows, 
                m_cols
            );
            GridBFSState empty_state;
            empty_state.is_complete = true;
            empty_state.phase = GridBFSState::Phase::COMPLETED;
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
            GridBFSState empty_state;
            empty_state.is_complete = true;
            empty_state.phase = GridBFSState::Phase::COMPLETED;
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

        // Initialize state
        GridBFSState state;
        state.phase = GridBFSState::Phase::INITIALIZE;
        state.visited_count = 0;
        state.comparisons = 0;
        state.is_complete = false;
        state.target_found = false;

        push_step(state, "init");

        // Push start node to queue
        m_cells[m_start.row][m_start.col].distance = 0;
        m_cells[m_start.row][m_start.col].in_queue = true;
        state.queue.push(m_start);
        m_frontier_order.push_back(m_start);

        state.phase = GridBFSState::Phase::PUSH_START;
        push_step(state, "push_start");

        // Process GridBFS until complete
        while (!state.is_complete && !state.target_found && !state.queue.empty())
        {
            // Pop front cell from queue
            GridPosition current_pos = state.queue.front();
            state.queue.pop();
            m_cells[current_pos.row][current_pos.col].in_queue = false;
            state.current_pos = current_pos;

            state.phase = GridBFSState::Phase::POP_FRONT;
            push_step(state, "pop_front");

            // Mark as visited (processed)
            if (!m_cells[current_pos.row][current_pos.col].visited)
            {
                m_cells[current_pos.row][current_pos.col].visited = true;
                state.visited_count++;
                m_visited_order.push_back(current_pos);
            }

            state.phase = GridBFSState::Phase::ADD_TO_VISITED;
            push_step(state, "add_to_visited");

            // Goal check
            state.phase = GridBFSState::Phase::GOAL_CHECK;
            push_step(state, "goal_check");

            if (current_pos == m_target)
            {
                state.target_found = true;
                reconstruct_path();
                state.phase = GridBFSState::Phase::TARGET_FOUND;
                push_step(state, "target_found");
                break;
            }

            // Evaluate all neighbors
            auto neighbors = get_neighbors(current_pos);

            for (const auto& neighbor : neighbors)
            {
                state.current_neighbor = neighbor;
                state.comparisons++;

                state.phase = GridBFSState::Phase::EVALUATE_NEIGHBOR;
                push_step(state, "evaluate_neighbor");

                // Skip if already visited or already in queue
                if (m_cells[neighbor.row][neighbor.col].visited ||
                    m_cells[neighbor.row][neighbor.col].in_queue)
                {
                    continue;
                }

                state.phase = GridBFSState::Phase::DISCOVER_NEIGHBOR;
                push_step(state, "discover_neighbor");

                // Discover new node
                m_cells[neighbor.row][neighbor.col].parent = current_pos;
                m_cells[neighbor.row][neighbor.col].distance =
                    m_cells[current_pos.row][current_pos.col].distance + 1;
                m_cells[neighbor.row][neighbor.col].in_queue = true;
                state.queue.push(neighbor);
                m_frontier_order.push_back(neighbor);

                state.phase = GridBFSState::Phase::PUSH_NEIGHBOR;
                push_step(state, "push_neighbor");
            }

            state.current_neighbor = GridPosition(-1, -1);
        }

        // Final steps
        if (state.queue.empty() && !state.target_found)
        {
            state.is_complete = true;
            state.phase = GridBFSState::Phase::QUEUE_EMPTY;
            push_step(state, "queue_empty");
        }

        if (state.target_found)
        {
            reconstruct_path();
        }

        state.is_complete = true;
        state.phase = GridBFSState::Phase::COMPLETED;
        push_step(state, "completed");

        LOG_DEBUG("Generated {} steps for GridBFS", m_steps.size());
    }

    void GridBFS::push_step(
        const GridBFSState& state, 
        const std::string& operation_id)
    {
        auto step = create_step_from_state(state, operation_id);
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep GridBFS::create_step_from_state(
        const GridBFSState& state, 
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

    void GridBFS::populate_step_metadata(
        AlgorithmStep& step, 
        const GridBFSState& state, 
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
            "queue_size", 
            state.queue.size(), 
            "Current queue size"
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

            int distance = m_cells[state.current_pos.row][state.current_pos.col].distance;
            if (distance >= 0)
            {
                step.metadata.set(
                    "distance", 
                    distance, 
                    "Steps from start"
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
        }

        if (state.target_found)
        {
            step.metadata.set(
                "path_length", 
                m_path.size(), 
                "Length of found path"
            );
            int path_distance = m_cells[m_target.row][m_target.col].distance;
            if (path_distance >= 0)
            {
                step.metadata.set(
                    "path_steps", 
                    path_distance, 
                    "Number of steps in path"
                );
            }
        }

        // Phase as string
        std::string phase_str;
        switch (state.phase)
        {
            case GridBFSState::Phase::INITIALIZE: 
                phase_str = "Initializing"; break;
            case GridBFSState::Phase::PUSH_START: 
                phase_str = "Pushing Start Cell"; break;
            case GridBFSState::Phase::POP_FRONT: 
                phase_str = "Popping Front of Queue"; break;
            case GridBFSState::Phase::GOAL_CHECK: 
                phase_str = "Goal Check"; break;
            case GridBFSState::Phase::ADD_TO_VISITED: 
                phase_str = "Adding to Visited Set"; break;
            case GridBFSState::Phase::EVALUATE_NEIGHBOR: 
                phase_str = "Evaluating Neighbor"; break;
            case GridBFSState::Phase::DISCOVER_NEIGHBOR: 
                phase_str = "Discovering Neighbor"; break;
            case GridBFSState::Phase::PUSH_NEIGHBOR: 
                phase_str = "Pushing to Queue"; break;
            case GridBFSState::Phase::TARGET_FOUND: 
                phase_str = "Target Found!"; break;
            case GridBFSState::Phase::QUEUE_EMPTY: 
                phase_str = "Queue Empty - No Path"; break;
            case GridBFSState::Phase::COMPLETED: 
                phase_str = "Completed"; break;
        }
        step.metadata.set(
            "phase", 
            phase_str, 
            "Current GridBFS phase"
        );
    }

    void GridBFS::update_visualization_data(
        AlgorithmStep& step, 
        const GridBFSState& state, 
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
                    if (m_cells[row][col].distance >= 0)
                    {
                        viz.graph_state.node_distances[to_node_id(row, col)] =
                            m_cells[row][col].distance;
                    }
                }
            }
        }

        // Mark frontier cells (in queue)
        for (int row = 0; row < m_rows; ++row)
        {
            for (int col = 0; col < m_cols; ++col)
            {
                if (m_cells[row][col].in_queue && !m_cells[row][col].visited)
                {
                    viz.graph_state.frontier_nodes.push_back(
                        to_node_id(row, col)
                    );
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
        if (state.current_neighbor.row >= 0 && 
            state.current_pos.row >= 0)
        {
            viz.graph_state.active_edges.emplace_back(
                to_node_id(state.current_pos.row, state.current_pos.col),
                to_node_id(state.current_neighbor.row, state.current_neighbor.col)
            );
            viz.additional_highlights.push_back(to_node_id(
                state.current_neighbor.row, 
                state.current_neighbor.col)
        );
        }

        viz.comparison_count = state.comparisons;
        viz.visited_node_count = state.visited_count;
        viz.is_complete = state.is_complete || state.target_found;
    }

    void GridBFS::reset_state()
    {
        m_current_state = GridBFSState{};
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