//
// Created by Akhmad on 5/22/26.
//

#include "algorithms/core/grid_pathfinding_base.hpp"
#include "core/utils/variables.hpp"

namespace c2l::algorithms
{
    GridPathfindingBase::GridPathfindingBase(
            core::JsonConfigManager& json_config_manager,
            AlgorithmType type)
        : JsonAlgorithmBase(json_config_manager, type)
    {
        // Initialize with default grid to ensure valid state
        m_rows = 20;
        m_cols = 20;
        m_grid.assign(20, std::vector<GridCellType>(20, GridCellType::EMPTY));
        m_cells.assign(20, std::vector<GridCell>(20));

        // Initialize cells with default values
        for (int row = 0; row < 20; ++row)
        {
            for (int col = 0; col < 20; ++col)
            {
                m_cells[row][col] = GridCell(row, col);
            }
        }

        // Set default start and target positions
        m_start = GridPosition(5, 5);
        m_target = GridPosition(14, 14);
    }

    void GridPathfindingBase::set_grid(int rows, int cols, const std::vector<int>& grid_data)
    {
        m_rows = rows;
        m_cols = cols;
        m_grid.assign(rows, std::vector<GridCellType>(cols, GridCellType::EMPTY));

        for (int row = 0; row < rows && row * cols < static_cast<int>(grid_data.size()); ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                int val = grid_data[row * cols + col];
                if (val == 1) m_grid[row][col] = GridCellType::WALL;
                else if (val >= 10 && val <= 14)
                    m_grid[row][col] = static_cast<GridCellType>(val);
                else m_grid[row][col] = GridCellType::EMPTY;
            }
        }

        reset_state();
    }

    void GridPathfindingBase::set_grid_cells(
        int rows,
        int cols,
        const std::vector<std::vector<GridCellType>>& grid)
    {
        m_rows = rows;
        m_cols = cols;
        m_grid = grid;
        reset_state();
    }

    void GridPathfindingBase::set_start(int row, int col)
    {
        if (row >= 0 && row < m_rows && col >= 0 && col < m_cols)
        {
            m_start = GridPosition(row, col);
        }
    }

    void GridPathfindingBase::set_target(int row, int col)
    {
        if (row >= 0 && row < m_rows && col >= 0 && col < m_cols)
        {
            m_target = GridPosition(row, col);
        }
    }

    void GridPathfindingBase::reconstruct_path()
    {
        m_path.clear();

        if (m_start == m_target)
        {
            m_path.push_back(m_start);
            return;
        }

        GridPosition current = m_target;

        while (current != m_start && current.row != -1 && current.col != -1)
        {
            m_path.push_back(current);
            current = m_cells[current.row][current.col].parent;
        }

        if (current == m_start)
        {
            m_path.push_back(m_start);
        }

        std::reverse(m_path.begin(), m_path.end());
    }

    bool GridPathfindingBase::is_walkable(int row, int col) const
    {
        if (row < 0 || row >= m_rows || col < 0 || col >= m_cols)
            return false;
        return m_grid[row][col] != GridCellType::WALL;
    }

    [[nodiscard]] float GridPathfindingBase::get_g_score(int row, int col) const
    {
        return m_cells[row][col].g_score;
    }
    [[nodiscard]] float GridPathfindingBase::get_f_score(int row, int col) const
    {
        return m_cells[row][col].f_score;
    }

    int GridPathfindingBase::get_cell_weight(const GridPosition& pos) const
    {
        if (!is_valid_position(pos))
            return 1;

        auto type = m_grid[pos.row][pos.col];
        switch (type)
        {
            case GridCellType::WEIGHT_1: return 1;
            case GridCellType::WEIGHT_2: return 2;
            case GridCellType::WEIGHT_3: return 3;
            case GridCellType::WEIGHT_4: return 4;
            case GridCellType::WEIGHT_5: return 5;
            case GridCellType::WALL: return static_cast<int>(core::INF);
            default: return 1;
        }
    }


    bool GridPathfindingBase::is_valid_position(const GridPosition& pos) const
    {
        return pos.row >= 0 && pos.row < m_rows &&
               pos.col >= 0 && pos.col < m_cols;
    }

    float GridPathfindingBase::get_distance(int row, int col) const
    {
        return m_cells[row][col].distance;
    }



}
