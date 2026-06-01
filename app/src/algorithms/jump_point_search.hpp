#ifndef CODE2LOGIC_JUMP_POINT_SEARCH_HPP
#define CODE2LOGIC_JUMP_POINT_SEARCH_HPP

#include "core/grid_pathfinding_base.hpp"
#include "algorithms/core/algorithm_step.hpp"
#include "algorithms/visualizers/grid_cell_type.hpp"
#include "algorithms/visualizers/grid_structures.hpp"

#include <queue>
#include <vector>


namespace c2l::algorithms
{
    /**
     * @brief Jump Point Search Algorithm
     *
     * JPS is an optimization of A* for uniform-cost grids that uses
     * symmetry reduction to prune redundant paths by "jumping" over
     * large sections of the grid.
     */
    class JumpPointSearch final : public GridPathfindingBase
    {
    public:
        explicit JumpPointSearch(core::JsonConfigManager& json_config_manager);
        ~JumpPointSearch() override = default;

        JumpPointSearch(const JumpPointSearch&) = delete;
        JumpPointSearch& operator=(const JumpPointSearch&) = delete;
        JumpPointSearch(JumpPointSearch&&) noexcept = delete;
        JumpPointSearch& operator=(JumpPointSearch&&) noexcept = delete;

        void generate_all_steps() override;
        void reset_state() override;

        [[nodiscard]] const std::vector<GridPosition>& get_jump_points() const 
        { 
            return m_jump_points; 
        }
        [[nodiscard]] GridPosition get_current_node() const 
        { 
            return m_current_state.current_pos; 
        }

    protected:
        float calculate_heuristic(
            const GridPosition& pos, 
            const GridPosition& target
        ) const override;
        float get_movement_cost(
            const GridPosition& from, 
            const GridPosition& to
        ) const override;

    private:
        struct JPSState
        {
            using PQElement = std::pair<float, GridPosition>;
            struct ComparePQ
            {
                bool operator()(const PQElement& a, const PQElement& b) const
                {
                    return a.first > b.first;
                }
            };
            std::priority_queue<PQElement, std::vector<PQElement>, ComparePQ> open_set;

            GridPosition current_pos{-1, -1};
            GridPosition current_jump_point{-1, -1};
            GridPosition current_neighbor{-1, -1};
            float current_edge_weight{1.0f};
            float tentative_g_score{0.0f};
            float old_g_score{0.0f};
            float new_f_score{0.0f};
            float new_g_score{0.0f};

            size_t explored_count{0};
            size_t comparisons{0};
            size_t jump_count{0};
            bool is_complete{false};
            bool target_found{false};

            enum class Phase
            {
                INITIALIZE,
                PUSH_START,
                POP_MIN,
                GOAL_CHECK,
                ADD_TO_CLOSED,
                FIND_JUMP_POINTS,
                PRUNE_NEIGHBORS,
                EVALUATE_JUMP_POINT,
                RELAX_EDGE,
                UPDATE_SCORES,
                PUSH_NEIGHBOR,
                TARGET_FOUND,
                OPEN_SET_EMPTY,
                COMPLETED
            } phase{Phase::INITIALIZE};
        };

        // Core JPS algorithms
        std::vector<GridPosition> get_pruned_neighbors(
            const GridPosition& pos, 
            const GridPosition& parent
        );
        GridPosition jump(
            const GridPosition& from, 
            const GridPosition& direction, 
            const GridPosition& target
        );
        bool has_forced_neighbor(
            const GridPosition& pos, 
            const GridPosition& direction
        ) const;

        // Helper methods
        std::vector<GridPosition> get_pruned_directions(
            const GridPosition& pos, 
            const GridPosition& parent
        ) const;
        void push_step(
            const JPSState& state, 
            const std::string& operation_id
        );
        AlgorithmStep create_step_from_state(
            const JPSState& state, 
            const std::string& operation_id
        ) const;

        void append_interpolated_path(
            std::vector<GridPosition>& path,
            const GridPosition& from,
            const GridPosition& to);

        void populate_step_metadata(
            AlgorithmStep& step, 
            const JPSState& state, 
            const std::string& operation_id
        ) const;
        void update_visualization_data(
            AlgorithmStep& step, 
            const JPSState& state, 
            const std::string& operation_id
        ) const;


        // Algorithm settings
        JPSState m_current_state;
        std::vector<GridPosition> m_jump_points;

        static constexpr int DIRS[8][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_JUMP_POINT_SEARCH_HPP