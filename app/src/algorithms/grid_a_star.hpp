#ifndef CODE2LOGIC_GRID_ASTAR_HPP
#define CODE2LOGIC_GRID_ASTAR_HPP

#include "algorithms/core/json_algorithm_base.hpp"
#include "algorithms/core/algorithm_step.hpp"
#include "algorithms/visualizers/grid_structures.hpp"

#include <queue>
#include <vector>

#include "core/grid_pathfinding_base.hpp"

namespace c2l::algorithms
{
    /**
     * @brief Grid-based A* Search Algorithm
     *
     * A* implementation specifically designed for grid-based pathfinding
     * with support for weighted terrain and diagonal movement.
     */
    class GridAStar final : public GridPathfindingBase
    {
    public:
        explicit GridAStar(core::JsonConfigManager& json_config_manager);
        ~GridAStar() override = default;

        GridAStar(const GridAStar&) = delete;
        GridAStar& operator=(const GridAStar&) = delete;
        GridAStar(GridAStar&&) noexcept = delete;
        GridAStar& operator=(GridAStar&&) noexcept = delete;

        void reset_state() override;
        void generate_all_steps() override;

    protected:
        float calculate_heuristic(
            const GridPosition& pos, 
            const GridPosition& target
        ) const override;
        float get_movement_cost(
            const GridPosition& from, 
            const GridPosition& to
        ) const override;
        std::vector<GridPosition> get_neighbors(
            const GridPosition& pos
        ) const override;

    private:
        struct GridAStarState
        {
            using PQElement = std::pair<float, GridPosition>;  // (f_score, position)
            struct ComparePQ
            {
                bool operator()(const PQElement& a, const PQElement& b) const
                {
                    return a.first > b.first;
                }
            };
            std::priority_queue<PQElement, std::vector<PQElement>, ComparePQ> open_set;

            GridPosition current_pos{-1, -1};
            GridPosition current_neighbor{-1, -1};
            float current_edge_weight{1.0f};
            float tentative_g_score{0.0f};
            float old_g_score{0.0f};
            float new_f_score{0.0f};

            size_t explored_count{0};
            size_t comparisons{0};
            bool is_complete{false};
            bool target_found{false};

            enum class Phase
            {
                INITIALIZE,
                PUSH_START,
                POP_MIN,
                GOAL_CHECK,
                ADD_TO_CLOSED,
                EVALUATE_NEIGHBOR,
                RELAX_EDGE,
                UPDATE_SCORES,
                PUSH_NEIGHBOR,
                TARGET_FOUND,
                OPEN_SET_EMPTY,
                COMPLETED
            } phase{Phase::INITIALIZE};
        };

        void push_step(
            const GridAStarState& state, 
            const std::string& operation_id
        );
        AlgorithmStep create_step_from_state(
            const GridAStarState& state, 
            const std::string& operation_id
        ) const;
        void populate_step_metadata(
            AlgorithmStep& step, 
            const GridAStarState& state, 
            const std::string& operation_id
        ) const;
        void update_visualization_data(
            AlgorithmStep& step, 
            const GridAStarState& state, 
            const std::string& operation_id
        ) const;

        // Algorithm settings
        GridAStarState m_current_state;
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_GRID_ASTAR_HPP