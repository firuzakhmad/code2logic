#ifndef CODE2LOGIC_GRID_BFS_HPP
#define CODE2LOGIC_GRID_BFS_HPP

#include "algorithms/core/grid_pathfinding_base.hpp"
#include "algorithms/core/algorithm_step.hpp"
#include "algorithms/visualizers/grid_structures.hpp"

#include <queue>
#include <vector>

namespace c2l::algorithms
{
    /**
     * @brief Breadth-First Search Algorithm for Grid Pathfinding
     *
     * GridBFS explores the grid level by level, guaranteeing the shortest path
     * in terms of number of steps (edges) on unweighted grids.
     *
     * BFS uses a queue (FIFO) and explores all nodes at the current depth
     * before moving to the next depth level.
     */
    class GridBFS final : public GridPathfindingBase
    {
    public:
        explicit GridBFS(core::JsonConfigManager& json_config_manager);
        ~GridBFS() override = default;

        GridBFS(const GridBFS&) = delete;
        GridBFS& operator=(const GridBFS&) = delete;
        GridBFS(GridBFS&&) noexcept = delete;
        GridBFS& operator=(GridBFS&&) noexcept = delete;

        void reset_state() override;
        void generate_all_steps() override;

        std::vector<GridPosition> get_neighbors(
            const GridPosition& pos
        ) const override;

        // BFS-specific getters
        [[nodiscard]] size_t get_queue_size() const 
        { 
            return m_current_state.queue.size(); 
        }

    private:
        struct GridBFSState
        {
            std::queue<GridPosition> queue;

            GridPosition current_pos{-1, -1};
            GridPosition current_neighbor{-1, -1};

            size_t visited_count{0};
            size_t comparisons{0};
            bool is_complete{false};
            bool target_found{false};

            enum class Phase
            {
                INITIALIZE,
                PUSH_START,
                POP_FRONT,
                GOAL_CHECK,
                ADD_TO_VISITED,
                EVALUATE_NEIGHBOR,
                DISCOVER_NEIGHBOR,
                PUSH_NEIGHBOR,
                TARGET_FOUND,
                QUEUE_EMPTY,
                COMPLETED
            } phase{Phase::INITIALIZE};
        };

        void push_step(
            const GridBFSState& state, 
            const std::string& operation_id
        );
        AlgorithmStep create_step_from_state(
            const GridBFSState& state, 
            const std::string& operation_id
        ) const;
        void populate_step_metadata(
            AlgorithmStep& step, 
            const GridBFSState& state, 
            const std::string& operation_id
        ) const;
        void update_visualization_data(
            AlgorithmStep& step, 
            const GridBFSState& state, 
            const std::string& operation_id
        ) const;

        GridBFSState m_current_state;
    };
} // namespace c2l::algorithms

#endif // CODE2LOGIC_GRID_BFS_HPP