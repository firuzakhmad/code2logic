//
// Created by Akhmad on 5/20/26.
//

#ifndef CODE2LOGIC_THETA_STAR_HPP
#define CODE2LOGIC_THETA_STAR_HPP

#include "algorithms/core/grid_pathfinding_base.hpp"
#include "algorithms/core/algorithm_step.hpp"
#include "algorithms/visualizers/grid_cell_type.hpp"
#include "algorithms/visualizers/grid_structures.hpp"

#include <queue>
#include <vector>
#include <cmath>
#include <functional>

#include "core/grid_pathfinding_base.hpp"

namespace c2l::algorithms
{
    /**
     * @brief Theta* Algorithm for Any-Angle Grid Pathfinding
     *
     * Theta* is an extension of A* that allows paths to cut across grid cells
     * using line-of-sight checks, resulting in shorter and more natural paths.
     */
    class ThetaStar final : public GridPathfindingBase
    {
    public:
        explicit ThetaStar(core::JsonConfigManager& json_config_manager);
        ~ThetaStar() override = default;

        ThetaStar(const ThetaStar&) = delete;
        ThetaStar& operator=(const ThetaStar&) = delete;
        ThetaStar(ThetaStar&&) noexcept = delete;
        ThetaStar& operator=(ThetaStar&&) noexcept = delete;

        void generate_all_steps() override;
        void reset_state() override;

        void set_line_of_sight_check(bool enable) 
        { 
            m_enable_line_of_sight = enable; 
        }

        // Get results for visualization
        [[nodiscard]] GridPosition get_current_node() const 
        { 
            return m_current_state.current_pos; 
        }
    protected:
        float calculate_heuristic(
            const GridPosition& pos, 
            const GridPosition& target
        ) const override;
        std::vector<GridPosition> get_neighbors(
            const GridPosition& pos
        ) const override;
        float get_movement_cost(
            const GridPosition& from, 
            const GridPosition& to
        ) const override;

    private:
        struct ThetaState
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
            GridPosition current_neighbor{-1, -1};
            GridPosition line_of_sight_parent{-1, -1};
            float current_edge_weight{1.0f};
            float tentative_g_score{0.0f};
            float old_g_score{0.0f};
            float new_f_score{0.0f};
            bool line_of_sight_success{false};

            size_t explored_count{0};
            size_t comparisons{0};
            size_t line_of_sight_checks{0};
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
                LINE_OF_SIGHT_CHECK,
                UPDATE_WITH_PARENT,
                RELAX_EDGE,
                UPDATE_SCORES,
                PUSH_NEIGHBOR,
                TARGET_FOUND,
                OPEN_SET_EMPTY,
                COMPLETED
            } phase{Phase::INITIALIZE};
        };

        // Core Theta* methods
        float get_euclidean_distance(
            const GridPosition& from, 
            const GridPosition& to
        ) const;
        bool has_line_of_sight(
            const GridPosition& from, 
            const GridPosition& to
        ) const;

        void push_step(
            const ThetaState& state, 
            const std::string& operation_id
        );
        AlgorithmStep create_step_from_state(
            const ThetaState& state, 
            const std::string& operation_id
        ) const;
        void populate_step_metadata(
            AlgorithmStep& step, 
            const ThetaState& state, 
            const std::string& operation_id
        ) const;
        void update_visualization_data(
            AlgorithmStep& step, 
            const ThetaState& state, 
            const std::string& operation_id
        ) const;

        // Algorithm settings
        bool m_enable_line_of_sight{true};
        ThetaState m_current_state;

        static constexpr int DIRS[8][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1},
                                           {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    };

} // namespace c2l::algorithms

#endif //CODE2LOGIC_THETA_STAR_HPP