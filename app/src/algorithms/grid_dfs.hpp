//
// Created by Akhmad on 5/20/26.
//

#ifndef CODE2LOGIC_GRID_DFS_HPP
#define CODE2LOGIC_GRID_DFS_HPP

#include "algorithms/core/algorithm_step.hpp"
#include "algorithms/visualizers/grid_structures.hpp"

#include <stack>
#include <vector>

#include "core/grid_pathfinding_base.hpp"

namespace c2l::algorithms
{
    /**
     * @brief Depth-First Search Algorithm for Grid Pathfinding
     *
     * DFS explores as far as possible along each branch before backtracking.
     * It uses a stack (LIFO) for frontier management.
     */
    class GridDFS final : public GridPathfindingBase
    {
    public:
        explicit GridDFS(core::JsonConfigManager& json_config_manager);
        ~GridDFS() override = default;

        GridDFS(const GridDFS&) = delete;
        GridDFS& operator=(const GridDFS&) = delete;
        GridDFS(GridDFS&&) noexcept = delete;
        GridDFS& operator=(GridDFS&&) noexcept = delete;

        void generate_all_steps() override;
        void reset_state() override;

        void set_use_recursive(bool recursive) { m_use_recursive = recursive; }

        // Get results for visualization
        [[nodiscard]] GridPosition get_current_node() const { return m_current_state.current_pos; }
        [[nodiscard]] size_t get_stack_size() const { return m_current_state.stack.size(); }
        [[nodiscard]] size_t get_visited_count() const { return m_current_state.visited_count; }

    protected:
        std::vector<GridPosition> get_neighbors(const GridPosition& pos) const override;

    private:
        struct GridDFSState
        {
            std::stack<GridPosition> stack;

            GridPosition current_pos{-1, -1};
            GridPosition current_neighbor{-1, -1};
            int current_depth{0};

            size_t visited_count{0};
            size_t comparisons{0};
            int time_counter{0};
            bool is_complete{false};
            bool target_found{false};

            enum class Phase
            {
                INITIALIZE,
                PUSH_START,
                POP_TOP,
                GOAL_CHECK,
                ADD_TO_VISITED,
                EVALUATE_NEIGHBOR,
                DISCOVER_NEIGHBOR,
                PUSH_NEIGHBOR,
                BACKTRACK,
                TARGET_FOUND,
                STACK_EMPTY,
                COMPLETED
            } phase{Phase::INITIALIZE};
        };

        // Core GridDFS methods
        void push_step(const GridDFSState& state, const std::string& operation_id);
        AlgorithmStep create_step_from_state(const GridDFSState& state, const std::string& operation_id) const;
        void populate_step_metadata(AlgorithmStep& step, const GridDFSState& state, const std::string& operation_id) const;
        void update_visualization_data(AlgorithmStep& step, const GridDFSState& state, const std::string& operation_id) const;

        bool m_use_recursive{false};  // Use iterative stack-based GridDFS by default
        GridDFSState m_current_state;

        static constexpr int DIRS[8][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1},
                                           {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    };

} // namespace c2l::algorithms

#endif //CODE2LOGIC_GRID_DFS_HPP