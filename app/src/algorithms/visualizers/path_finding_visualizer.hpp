#ifndef CODE2LOGIC_PATH_FINDING_VISUALIZER_HPP
#define CODE2LOGIC_PATH_FINDING_VISUALIZER_HPP

#include "algorithms/visualizers/i_algorithm_visualizer.hpp"
#include "algorithms/core/algorithm_metadata_types.hpp"
#include "algorithms/visualizers/visualization_style.hpp"
#include "ui/managers/icon_manager.hpp"
#include "algorithms/grid_a_star.hpp"
#include "algorithms/core/algorithm_observer.hpp"
#include "algorithms/visualizers/grid_cell_type.hpp"
#include "algorithms/visualizers/grid_cell_data.hpp"

#include <imgui.h>
#include <vector>

namespace c2l::algorithms
{
    class GridAStar;

    struct PathFindingState
    {
        int rows{20};
        int cols{20};
        GridPosition start{-1, -1};
        GridPosition target{-1, -1};
        std::vector<std::vector<GridCellData>> grid;

        // Algorithm state
        std::vector<GridPosition> visited_order;
        std::vector<GridPosition> frontier_order;
        std::vector<GridPosition> path;
        GridPosition current_node{-1, -1};
        size_t comparisons{0};
        size_t path_cost{0};
        bool is_complete{false};
        bool path_found{false};
        float algorithm_time{0.0f};
    };

    class PathFindingVisualizer final : public IAlgorithmVisualizer, public AlgorithmObserver
    {
    public:
        explicit PathFindingVisualizer(
            ui::managers::IconManager& icon_manager, 
            const VisualizationConfig& config = {});
        ~PathFindingVisualizer() override = default;

        void initialize(
            ISimpleAlgorithm* execution, 
            const IAlgorithmMetadata* metadata,
            const bool show_sidebar_controller
        ) override;
        void update(double delta_time) override;
        void render() override;
        void set_visualization_style(VisualizationStyle style) override;
        [[nodiscard]] VisualizationType get_visualization_type() const override;
        [[nodiscard]] bool supports_algorithm(const AlgorithmType& type) const override;
        [[nodiscard]] size_t get_total_visited_nodes() const override;
        [[nodiscard]] size_t get_total_explored_nodes() const override;

        // AlgorithmObserver implementation
        void on_step_changed() override;

        // Grid editing (no playback controls)
        void set_grid_size(int rows, int cols);
        void clear_grid();
        void generate_random_maze(float wall_density = 0.3f);
        void generate_recursive_backtracking_maze();
        void set_start(int row, int col);
        void set_target(int row, int col);
        void reset_visualization();

        void set_show_grid_lines(bool show) { m_show_grid_lines = show; }
        void set_show_weights(bool show) { m_show_weights = show; }
        void set_show_coordinates(bool show) { m_show_coordinates = show; }
        void set_wall(int row, int col, bool is_wall);
        void set_weight(int row, int col, int weight);

        void sync_algorithm_with_grid();

    private:
        void render_grid();
        void render_cell(
            int row, 
            int col, 
            const ImVec2& cell_pos, 
            float cell_size
        );
        void render_grid_editor_panel();
        void render_info_panel();
        void render_legend_panel();
        void handle_mouse_interaction();

        GridCellData &cell_at(int row, int col);

        [[nodiscard]] const GridCellData &cell_at(int row, int col) const;

        void update_algorithm_state();

        ui::managers::IconManager& m_icon_manager;
        const VisualizationConfig& m_config;

        ISimpleAlgorithm* m_execution{nullptr};
        const IAlgorithmMetadata* m_metadata{nullptr};

        PathFindingState m_state;

        // UI State
        bool m_show_grid_lines{true};
        bool m_show_weights{true};
        bool m_show_coordinates{false};

        // Interaction
        GridToolMode m_current_tool{GridToolMode::SELECT};

        std::pair<int, int> m_hovered_cell{-1, -1};
        bool m_is_dragging{false};
    };
} // namespace c2l::algorithms

#endif