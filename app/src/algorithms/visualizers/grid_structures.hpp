#ifndef CODE2LOGIC_GRID_STRUCTURES_HPP
#define CODE2LOGIC_GRID_STRUCTURES_HPP

#include "core/utils/variables.hpp"

#include <vector>

namespace c2l::algorithms
{
    /**
     * @brief Represents a position in a 2D grid
     */
    struct GridPosition
    {
        int row;
        int col;

        GridPosition() : row(-1), col(-1) {}
        GridPosition(const int r, const int c)
            : row(r), col(c)
        {}

        bool operator==(const GridPosition& other) const
        {
            return row == other.row && col == other.col;
        }

        bool operator!=(const GridPosition& other) const
        {
            return !(*this == other);
        }

        GridPosition operator+(const GridPosition& other) const
        {
            return {row + other.row, col + other.col};
        }

        [[nodiscard]] bool is_valid(const int rows, const int cols) const
        {
            return row >= 0 && row < rows && col >= 0 && col < cols;
        }
    };

    /**
     * @brief Represents a cell in a grid for pathfinding algorithms
     */
    struct GridCell
    {
        GridPosition pos;
        float g_score;          // Cost from start to this cell
        float f_score;          // g_score + heuristic (estimated total cost)
        float h_score;          // Heuristic value (estimated distance to target)
        GridPosition parent;    // Parent cell in the path
        bool visited;           // In closed set (already processed)
        bool in_open;           // In open set (discovered but not processed)
        int weight;             // Terrain weight/cost multiplier

        // BFS specific fields
        float distance;       // Number of steps from start (for BFS)
        bool in_queue;

        // DFS specific fields
        bool in_stack;
        int discovery_time;
        int finish_time;

        // Jump-Point Search specific fields
        bool is_jump_point;


        GridCell()
            : pos{-1, -1}
            , g_score{core::INF}
            , f_score{core::INF}
            , h_score{core::INF}
            , parent{-1, -1}
            , visited{false}
            , in_open{false}
            , weight{1}
            , distance{core::INF}
            , in_queue{false}
            , in_stack{false}
            , discovery_time{-1}
            , finish_time{-1}
            , is_jump_point{false}
        {}

        explicit GridCell(const int row, const int col)
            : pos{row, col}
            , g_score{core::INF}
            , f_score{core::INF}
            , h_score{core::INF}
            , parent{-1, -1}
            , visited{false}
            , in_open{false}
            , weight{1}
            , distance{core::INF}
            , in_queue{false}
            , in_stack{false}
            , discovery_time{-1}
            , finish_time{-1}
            , is_jump_point{false}
        {}

        void reset()
        {
            g_score = core::INF;
            f_score = core::INF;
            f_score = core::INF;
            parent = GridPosition(-1, -1);
            visited = false;
            in_open = false;
            distance= core::INF;
            in_queue = false;
            in_stack = false;
            discovery_time = -1;
            finish_time = -1;
            is_jump_point = false;
        }
    };

    /**
     * @brief Result data for pathfinding algorithms
     */
    struct PathfindingResult
    {
        std::vector<GridPosition> path;
        std::vector<GridPosition> visited_order;
        std::vector<GridPosition> frontier_order;
        float total_cost            {0.0f};
        size_t total_comparisons    {0};
        size_t nodes_explored       {0};
        size_t steps_taken          {0};
        size_t nodes_visited        {0};
        bool path_found             {false};
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_GRID_STRUCTURES_HPP
