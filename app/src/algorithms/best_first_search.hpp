#ifndef CODE2LOGIC_BEST_FIRST_SEARCH_HPP
#define CODE2LOGIC_BEST_FIRST_SEARCH_HPP

#include "algorithms/core/grid_pathfinding_base.hpp"
#include "algorithms/core/algorithm_step.hpp"
#include "algorithms/visualizers/grid_cell_type.hpp"
#include "algorithms/visualizers/grid_structures.hpp"

#include <queue>
#include <vector>
#include <unordered_map>
#include <functional>

namespace c2l::algorithms
{
    /**
     * @brief Greedy Best-First Search Algorithm for Grid Pathfinding
     *
     * Best-First Search uses only the heuristic function to guide the search,
     * prioritizing nodes that appear closest to the target. It is very fast
     * but does not guarantee the shortest path.
     */
    class BestFirstSearch final : public GridPathfindingBase
    {
    public:
        explicit BestFirstSearch(
            core::JsonConfigManager& json_config_manager
        );
        ~BestFirstSearch() override = default;

        BestFirstSearch(const BestFirstSearch&) = delete;
        BestFirstSearch& operator=(const BestFirstSearch&) = delete;
        BestFirstSearch(BestFirstSearch&&) noexcept = delete;
        BestFirstSearch& operator=(BestFirstSearch&&) noexcept = delete;

        void reset_state() override;
        void generate_all_steps() override;

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

        void set_tie_breaking(bool enable) { m_use_tie_breaking = enable; }

        // Get results for visualization
        [[nodiscard]] GridPosition get_current_node() const 
        { 
            return m_current_state.current_pos; 
        }
        [[nodiscard]] float get_heuristic_value(int row, int col) const 
        { 
            return m_cells[row][col].h_score; 
        }

    private:
        struct BestFirstState
        {
            using PQElement = std::pair<float, GridPosition>;
            struct ComparePQ
            {
                bool operator()(const PQElement& a, const PQElement& b) const
                {
                    // For tie-breaking, compare position if f-scores are equal
                    if (std::abs(a.first - b.first) < 0.0001f)
                    {
                        // Use position as tie-breaker for deterministic behavior
                        return a.second.row + a.second.col > b.second.row + b.second.col;
                    }
                    return a.first > b.first;
                }
            };
            std::priority_queue<PQElement, std::vector<PQElement>, ComparePQ> open_set;

            GridPosition current_pos{-1, -1};
            GridPosition current_neighbor{-1, -1};
            float current_edge_weight{1.0f};
            float current_heuristic{0.0f};
            float new_h_score{0.0f};

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
                ADD_TO_VISITED,
                EVALUATE_NEIGHBOR,
                COMPUTE_HEURISTIC,
                UPDATE_SCORES,
                PUSH_NEIGHBOR,
                TARGET_FOUND,
                OPEN_SET_EMPTY,
                COMPLETED
            } phase{Phase::INITIALIZE};
        };

        // Core Best-First Search methods
        void push_step(
            const BestFirstState& state, 
            const std::string& operation_id
        );
        AlgorithmStep create_step_from_state(
            const BestFirstState& state, 
            const std::string& operation_id
        ) const;
        void populate_step_metadata(
            AlgorithmStep& step, 
            const BestFirstState& state, 
            const std::string& operation_id
        ) const;
        void update_visualization_data(
            AlgorithmStep& step, 
            const BestFirstState& state, 
            const std::string& operation_id
        ) const;

        bool m_use_tie_breaking{true};
        BestFirstState m_current_state;

        static constexpr int DIRS[8][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1},
                                           {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_BEST_FIRST_SEARCH_HPP