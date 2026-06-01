//
// Created by Akhmad on 5/20/26.
//

#ifndef CODE2LOGIC_GRID_DIJKSTRA_HPP
#define CODE2LOGIC_GRID_DIJKSTRA_HPP

#include "algorithms/core/algorithm_step.hpp"
#include "algorithms/visualizers/grid_structures.hpp"
#include "core/grid_pathfinding_base.hpp"

#include <queue>
#include <vector>


namespace c2l::algorithms
{
    /**
     * @brief GridDijkstra's Algorithm for Weighted Grid Pathfinding
     *
     * GridDijkstra's algorithm finds the shortest path from a start node to all other nodes
     * in a weighted graph. It's the foundation for many pathfinding algorithms and
     * guarantees the shortest path when all edge weights are non-negative.
     */
    class GridDijkstra final : public GridPathfindingBase
    {
    public:
        explicit GridDijkstra(core::JsonConfigManager& json_config_manager);
        ~GridDijkstra() override = default;

        GridDijkstra(const GridDijkstra&) = delete;
        GridDijkstra& operator=(const GridDijkstra&) = delete;
        GridDijkstra(GridDijkstra&&) noexcept = delete;
        GridDijkstra& operator=(GridDijkstra&&) noexcept = delete;

        void generate_all_steps() override;
        void reset_state() override;

        // GridDijkstra-specific settings
        void set_stop_at_target(bool stop) 
        { 
            m_stop_at_target = stop; 
        }

        // Get results for visualization
        [[nodiscard]] GridPosition get_current_node() const 
        { 
            return m_current_state.current_pos; 
        }

    protected:
        float get_movement_cost(
            const GridPosition& from, 
            const GridPosition& to
        ) const override;
        std::vector<GridPosition> get_neighbors(
            const GridPosition& pos
        ) const override;

    private:
        struct GridDijkstraState
        {
            using PQElement = std::pair<float, GridPosition>;  // (distance, position)
            struct ComparePQ
            {
                bool operator()(const PQElement& a, const PQElement& b) const
                {
                    return a.first > b.first;  // Min-heap by distance
                }
            };
            std::priority_queue<PQElement, std::vector<PQElement>, ComparePQ> open_set;

            GridPosition current_pos{-1, -1};
            GridPosition current_neighbor{-1, -1};
            float current_edge_weight{1.0f};
            float tentative_distance{0.0f};
            float old_distance{0.0f};

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
                UPDATE_DISTANCE,
                PUSH_NEIGHBOR,
                TARGET_FOUND,
                OPEN_SET_EMPTY,
                COMPLETED
            } phase{Phase::INITIALIZE};
        };

        // Core GridDijkstra methods
        void push_step(
            const GridDijkstraState& state, 
            const std::string& operation_id
        );
        AlgorithmStep create_step_from_state(
            const GridDijkstraState& state, 
            const std::string& operation_id
        ) const;
        void populate_step_metadata(
            AlgorithmStep& step, 
            const GridDijkstraState& state, 
            const std::string& operation_id
        ) const;
        void update_visualization_data(
            AlgorithmStep& step, 
            const GridDijkstraState& state, 
            const std::string& operation_id
        ) const;

        bool m_stop_at_target{true};  // Stop when target is found
        GridDijkstraState m_current_state;

        static constexpr int DIRS[8][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1},
                                           {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    };

} // namespace c2l::algorithms

#endif //CODE2LOGIC_GRID_DIJKSTRA_HPP