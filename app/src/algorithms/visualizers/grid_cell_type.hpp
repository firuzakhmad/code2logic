//
// Created by Akhmad on 5/18/26.
//

#ifndef CODE2LOGIC_GRID_CELL_TYPE_HPP
#define CODE2LOGIC_GRID_CELL_TYPE_HPP

namespace c2l::algorithms
{
    enum class GridCellType
    {
        EMPTY = 0,
        WALL = 1,
        START = 2,
        TARGET = 3,
        VISITED = 4,
        FRONTIER = 5,
        PATH = 6,
        WEIGHT_1 = 10,
        WEIGHT_2 = 11,
        WEIGHT_3 = 12,
        WEIGHT_4 = 13,
        WEIGHT_5 = 14
    };
} // namespace c2l::algorithms

#endif //CODE2LOGIC_GRID_CELL_TYPE_HPP