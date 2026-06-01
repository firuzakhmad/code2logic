//
// Created by Akhmad on 5/30/26.
//

#ifndef CODE2LOGIC_SHARED_GRID_STATE_HPP
#define CODE2LOGIC_SHARED_GRID_STATE_HPP

#include "algorithms/visualizers/grid_cell_type.hpp"
#include "algorithms/visualizers/grid_structures.hpp"
#include "algorithms/visualizers/grid_cell_data.hpp"
#include <functional>
#include <random>

namespace c2l::algorithms
{
    /**
     * @brief Shared grid state for both left and right algorithms in comparison mode
     *
     * Reuses existing GridPosition, GridCellType, and GridToolMode
     */
    struct SharedGridState
    {
        int rows{20};
        int cols{20};
        std::vector<std::vector<GridCellType>> grid;
        GridPosition start{5, 5};
        GridPosition target{14, 14};

        // Callback when grid changes
        std::function<void()> on_grid_changed;

        SharedGridState()
        {
            resize(20, 20);
        }

        void resize(int new_rows, int new_cols)
        {
            rows = new_rows;
            cols = new_cols;
            grid.assign(rows, std::vector<GridCellType>(cols, GridCellType::EMPTY));
        }

        void clear()
        {
            for (int row = 0; row < rows; ++row)
                for (int col = 0; col < cols; ++col)
                    grid[row][col] = GridCellType::EMPTY;
        }

        void set_start(int row, int col)
        {
            if (start.row >= 0 && start.row < rows && start.col >= 0 && start.col < cols)
                grid[start.row][start.col] = GridCellType::EMPTY;

            start = GridPosition(row, col);
            if (row >= 0 && row < rows && col >= 0 && col < cols)
                grid[row][col] = GridCellType::START;
        }

        void set_target(int row, int col)
        {
            if (target.row >= 0 && target.row < rows && target.col >= 0 && target.col < cols)
                grid[target.row][target.col] = GridCellType::EMPTY;

            target = GridPosition(row, col);
            if (row >= 0 && row < rows && col >= 0 && col < cols)
                grid[row][col] = GridCellType::TARGET;
        }

        void set_wall(int row, int col, bool is_wall)
        {
            if (row < 0 || row >= rows || col < 0 || col >= cols) return;
            if (grid[row][col] == GridCellType::START || grid[row][col] == GridCellType::TARGET)
                return;
            grid[row][col] = is_wall ? GridCellType::WALL : GridCellType::EMPTY;
        }

        void set_weight(int row, int col, int weight)
        {
            if (row < 0 || row >= rows || col < 0 || col >= cols) return;
            if (grid[row][col] == GridCellType::START || grid[row][col] == GridCellType::TARGET)
                return;
            if (weight >= 1 && weight <= 5)
                grid[row][col] = static_cast<GridCellType>(
                    static_cast<int>(GridCellType::WEIGHT_1) + weight - 1);
        }

        void generate_random_maze(float density = 0.35f)
        {
            clear();

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(0.0, 1.0);

            for (int row = 0; row < rows; ++row)
            {
                for (int col = 0; col < cols; ++col)
                {
                    if ((row == start.row && col == start.col) ||
                        (row == target.row && col == target.col))
                        continue;

                    if (dis(gen) < density)
                        grid[row][col] = GridCellType::WALL;
                }
            }
        }

        void generate_recursive_backtracking_maze()
        {
            clear();

            // Fill with walls first
            for (int row = 0; row < rows; ++row)
            {
                for (int col = 0; col < cols; ++col)
                {
                    if ((row != start.row || col != start.col) &&
                        (row != target.row || col != target.col))
                    {
                        grid[row][col] = GridCellType::WALL;
                    }
                }
            }

            std::random_device rd;
            std::mt19937 gen(rd());

            std::function<void(int, int)> carve = [&](int row, int col)
            {
                std::vector<std::pair<int, int>> dirs = {{-2, 0}, {0, 2}, {2, 0}, {0, -2}};
                std::shuffle(dirs.begin(), dirs.end(), gen);

                for (auto [dr, dc] : dirs)
                {
                    int new_row = row + dr;
                    int new_col = col + dc;

                    if (new_row > 0 && new_row < rows - 1 &&
                        new_col > 0 && new_col < cols - 1 &&
                        grid[new_row][new_col] == GridCellType::WALL)
                    {
                        grid[new_row][new_col] = GridCellType::EMPTY;
                        grid[row + dr/2][col + dc/2] = GridCellType::EMPTY;
                        carve(new_row, new_col);
                    }
                }
            };

            carve(1, 1);
            set_start(1, 1);
            set_target(rows - 2, cols - 2);
        }

        std::vector<int> flatten() const
        {
            std::vector<int> flat;
            flat.reserve(rows * cols);
            for (int row = 0; row < rows; ++row)
                for (int col = 0; col < cols; ++col)
                    flat.push_back(static_cast<int>(grid[row][col]));
            return flat;
        }

        // Statistics
        int get_wall_count() const
        {
            int count = 0;
            for (int row = 0; row < rows; ++row)
                for (int col = 0; col < cols; ++col)
                    if (grid[row][col] == GridCellType::WALL)
                        count++;
            return count;
        }

        int get_weighted_count() const
        {
            int count = 0;
            for (int row = 0; row < rows; ++row)
                for (int col = 0; col < cols; ++col)
                    if (grid[row][col] >= GridCellType::WEIGHT_1 &&
                        grid[row][col] <= GridCellType::WEIGHT_5)
                        count++;
            return count;
        }
    };
}

#endif // CODE2LOGIC_SHARED_GRID_STATE_HPP