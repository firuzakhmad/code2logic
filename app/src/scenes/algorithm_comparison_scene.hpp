#ifndef CODE2LOGIC_ALGORITHM_COMPARISON_SCENE_HPP
#define CODE2LOGIC_ALGORITHM_COMPARISON_SCENE_HPP

#include "scenes/base_scene.hpp"
#include "algorithms/managers/parallel_comparison_manager.hpp"
#include "algorithms/visualizers/side_by_side_visualizer.hpp"
#include "core/utils/thread_manager/thread_manager.hpp"
#include "algorithms/core/algorithm_registry.hpp"
#include "algorithms/visualizers/shared_graph_state.hpp"
#include "algorithms/visualizers/shared_grid_state.hpp"

namespace c2l::scenes
{
    /**
     * @brief Scene for comparing two algorithms side by side
     */
    class AlgorithmComparisonScene final : public BaseScene
    {
    public:
        AlgorithmComparisonScene(
            graphics::Renderer& renderer,
            core::ThreadManager& thread_manager,
            core::JsonConfigManager& json_config_manager,
            algorithms::AlgorithmRegistry& algorithm_registry,
            core::resources::ResourceManager& resource_manager,
            ui::managers::IconManager& icon_manager
        );
            
        ~AlgorithmComparisonScene() override = default;

        // IScene interface
        void on_create() override;
        void on_destroy() override;
        void on_activate() override;
        void on_deactivate() override;
        void process_input(const core::InputHandler& input) override;
        void update(double dt) override;
        void render() override;

    private:
        // Panel rendering methods
        void render_selection_panel();
        void render_comparison_view_panel();
        void render_control_panel();

        void render_array_based_data_controls();
        void render_graph_based_data_controls();

        void render_shared_graph_editor();

        void sync_shared_graph_to_algorithms();

        std::string get_layout_name(algorithms::GraphLayoutEngine::LayoutType type);

        void sync_graph_display_options();

        void render_path_based_data_controls();

        void render_data_control_panel();
        void render_results_panel();
        void render_performance_graphs();
        
        // Helper methods
        void setup_main_menu() override;
        void sync_visualizers();
        void rebuild_category_cache();

        // Shared grid methods
        void sync_shared_grid_to_algorithms();
        void render_shared_grid_editor();
        void render_mini_grid_preview();
        void render_shared_grid_info();
        void render_shared_grid_legend();
        void handle_shared_grid_mouse_interaction(const ImVec2& grid_offset, float cell_size);


        // Cached algorithm info for UI
        struct AlgorithmInfo
        {
            std::string name;
            algorithms::AlgorithmType type;
            std::string category;
            std::string brief;
        };
        // Cached data
        std::unordered_map<
            std::string,
            std::vector<const algorithms::AlgorithmInfo*>
        > m_cached_categorized_algorithms;
        std::vector<std::string> m_cached_category_names;
        std::string m_selected_category;
        algorithms::VisualizationType m_selected_visualization_type;
        std::vector<const algorithms::AlgorithmInfo*> m_current_display_algorithms;
        bool m_cache_dirty{true};

        // Managers
        std::unique_ptr<algorithms::ParallelComparisonManager> m_comparison_manager;
        algorithms::SideBySideVisualizer m_visualizer;

        // Grid Based
        algorithms::SharedGridState m_shared_grid_state;
        algorithms::GridToolMode m_current_grid_tool{algorithms::GridToolMode::DRAW_WALLS};

        bool m_show_grid_lines{true};
        bool m_show_weights{true};
        bool m_show_coordinates{false};
        std::pair<int, int> m_hovered_cell{-1, -1};
        bool m_is_dragging{false};

        algorithms::SharedGraphState m_shared_graph_state;

        // Graph display options
        bool m_show_edge_weights{true};
        bool m_show_node_labels{true};
        bool m_show_node_ids{true};
        bool m_show_distances{true};


        // Selected algorithm types
        algorithms::AlgorithmType m_left_selected   { algorithms::AlgorithmType::BUBBLE_SORT };
        algorithms::AlgorithmType m_right_selected  { algorithms::AlgorithmType::QUICK_SORT };

        // UI state
        bool m_show_selection_panel     {true};
        bool m_show_comparison_view     {true};
        bool m_show_control_panel       {true};
        bool m_show_data_control        {true};
        bool m_show_results_panel       {true};
        bool m_show_performance_graphs  {false};
        
        // Data size control
        int m_data_size{15};
        int m_max_value{154};
        
        // Animation
        double m_animation_time{0.0};
    };

} // namespace c2l::scenes

#endif // CODE2LOGIC_ALGORITHM_COMPARISON_SCENE_HPP