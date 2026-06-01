//
// Created by Akhmad on 5/30/26.
//

#ifndef CODE2LOGIC_GRID_CELL_DATA_HPP
#define CODE2LOGIC_GRID_CELL_DATA_HPP

#include "algorithms/visualizers/grid_cell_type.hpp"
#include "algorithms/visualizers/grid_structures.hpp"


namespace c2l::algorithms
{
    struct GridCellData
    {
        GridCellType type           {GridCellType::EMPTY};
        int weight                  {1};
        int value                   {0};
        bool visited                {false};
        bool in_frontier            {false};
        bool on_path                {false};
        float g_score               {0.0f};
        float f_score               {0.0f};
        float h_score               {0.0f};
        GridPosition parent         {-1, -1};
        int distance                {-1};
        float animation_progress    {0.0f};
        float highlight_intensity   {0.0f};
    };

    // Interaction
    enum class GridToolMode
    {
        SELECT,
        DRAW_WALLS,
        DRAW_WEIGHTS,
        PLACE_START,
        PLACE_TARGET,
        ERASE
    };

    static constexpr const char* TOOL_NAMES[] =
    {
        "Select", "Walls", "Weights",
        "Start", "Target", "Erase"
    };

    static constexpr ImColor COLOR_EMPTY = ImColor(40, 45, 55, 255);
    static constexpr ImColor COLOR_WALL = ImColor(30, 35, 45, 255);
    static constexpr ImColor COLOR_START = ImColor(0, 200, 0, 255);
    static constexpr ImColor COLOR_TARGET = ImColor(255, 50, 50, 255);
    static constexpr ImColor COLOR_VISITED = ImColor(65, 105, 225, 200);
    static constexpr ImColor COLOR_FRONTIER = ImColor(255, 200, 50, 200);
    static constexpr ImColor COLOR_PATH = ImColor(155, 48, 255, 255);
    static constexpr ImColor COLOR_CURRENT = ImColor(255, 80, 80, 255);
    static constexpr ImColor COLOR_WEIGHT_1 = ImColor(50, 70, 90, 255);
    static constexpr ImColor COLOR_WEIGHT_2 = ImColor(60, 85, 110, 255);
    static constexpr ImColor COLOR_WEIGHT_3 = ImColor(70, 100, 130, 255);
    static constexpr ImColor COLOR_WEIGHT_4 = ImColor(80, 115, 150, 255);
    static constexpr ImColor COLOR_WEIGHT_5 = ImColor(90, 130, 170, 255);

    inline ImColor get_cell_color_for_preview(GridCellType type)
    {
        switch (type)
        {
            case GridCellType::START: return COLOR_START;
            case GridCellType::TARGET: return COLOR_TARGET;
            case GridCellType::WALL: return COLOR_WALL;
            case GridCellType::WEIGHT_1: return COLOR_WEIGHT_1;
            case GridCellType::WEIGHT_2: return COLOR_WEIGHT_2;
            case GridCellType::WEIGHT_3: return COLOR_WEIGHT_3;
            case GridCellType::WEIGHT_4: return COLOR_WEIGHT_4;
            case GridCellType::WEIGHT_5: return COLOR_WEIGHT_5;
            default: return COLOR_EMPTY;
        }
    }

}
#endif //CODE2LOGIC_GRID_CELL_DATA_HPP