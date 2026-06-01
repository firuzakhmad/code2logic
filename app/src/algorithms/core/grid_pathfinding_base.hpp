//
// Created by Akhmad on 5/22/26.
//

#ifndef CODE2LOGIC_GRID_PATH_FINDING_BASE_HPP
#define CODE2LOGIC_GRID_PATH_FINDING_BASE_HPP

#include "heuristic_type.hpp"
#include "algorithms/core/json_algorithm_base.hpp"
#include "algorithms/visualizers/grid_structures.hpp"
#include "algorithms/visualizers/grid_cell_type.hpp"

namespace c2l::algorithms
{
    /**
     * @brief Base class for all grid-based pathfinding algorithms
     *
     * Provides common functionality for:
     * - Grid management
     * - Path reconstruction
     * - Basic neighbor checking
     * - Terrain weight handling
     */
    class GridPathfindingBase : public JsonAlgorithmBase
    {
    public:
        explicit GridPathfindingBase(
            core::JsonConfigManager& json_config_manager,
            AlgorithmType type
        );
        ~GridPathfindingBase() override = default;

        // Grid configuration (shared by all)
        void set_grid(int rows, int cols, const std::vector<int>& grid_data) override;
        void set_grid_cells(int rows, int cols, const std::vector<std::vector<GridCellType>>& grid) override;
        void set_start(int row, int col) override;
        void set_target(int row, int col) override;
        void set_allow_diagonals(bool allow) override { m_allow_diagonals = allow; }
        void set_heuristic_type(HeuristicType type) override { m_heuristic_type = type; }

        // Common getters
        [[nodiscard]] const std::vector<GridPosition>& get_path() const override { return m_path; }
        [[nodiscard]] const std::vector<GridPosition>& get_visited_order() const override { return m_visited_order; }
        [[nodiscard]] const std::vector<GridPosition>& get_frontier_order() const override { return m_frontier_order; }
        [[nodiscard]] GridPosition get_current_node_grid() const override { return m_current_node; }
        [[nodiscard]] size_t get_open_set_size() const override { return m_open_set_size; }
        [[nodiscard]] size_t get_closed_set_size() const override { return m_closed_set_size; }
        [[nodiscard]] float get_distance(int row, int col) const override;


        [[nodiscard]] float get_g_score(int row, int col) const override;
        [[nodiscard]] float get_f_score(int row, int col) const override;

    protected:
        // Common pathfinding utilities
        void reconstruct_path();
        bool is_walkable(int row, int col) const;
        int get_cell_weight(const GridPosition& pos) const;
        bool is_valid_position(const GridPosition& pos) const;

        // Pure virtual methods that algorithms must implement
        virtual float calculate_heuristic(const GridPosition& pos, const GridPosition& target) const { return  0.0f; };
        virtual float get_movement_cost(const GridPosition& from, const GridPosition& to) const { return 0.0f; };
        virtual std::vector<GridPosition> get_neighbors(const GridPosition& pos) const { return{ pos }; };

        // Common grid data
        int m_rows{0};
        int m_cols{0};
        std::vector<std::vector<GridCellType>> m_grid;
        std::vector<std::vector<GridCell>> m_cells;
        GridPosition m_start{-1, -1};
        GridPosition m_target{-1, -1};

        // Results
        std::vector<GridPosition> m_path;
        std::vector<GridPosition> m_visited_order;
        std::vector<GridPosition> m_frontier_order;

        // Algorithm settings
        bool m_allow_diagonals{true};
        HeuristicType m_heuristic_type{HeuristicType::Euclidean};

        // State tracking (for getters)
        GridPosition m_current_node{-1, -1};
        size_t m_open_set_size{0};
        size_t m_closed_set_size{0};
    };
} // namespace c2l::algorithms

#endif //CODE2LOGIC_GRID_PATH_FINDING_BASE_HPP