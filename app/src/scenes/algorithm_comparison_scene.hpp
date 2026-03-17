#ifndef CODE2LOGIC_ALGORITHM_COMPARISON_SCENE_HPP
#define CODE2LOGIC_ALGORITHM_COMPARISON_SCENE_HPP

#include "scenes/base_scene.hpp"
#include "algorithms/managers/parallel_comparison_manager.hpp"
#include "algorithms/visualizers/side_by_side_visualizer.hpp"
#include "core/utils/thread_manager/thread_manager.hpp"
#include "algorithms/core/algorithm_registry.hpp"

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
        void render_data_control_panel();
        void render_results_panel();
        void render_performance_graphs();
        
        // Helper methods
        void setup_main_menu() override;
        void update_algorithm_cache();
        void sync_visualizers();

        // Cached algorithm info for UI
        struct AlgorithmInfo
        {
            std::string name;
            algorithms::AlgorithmType type;
            std::string category;
            std::string brief;
        };
        std::vector<AlgorithmInfo> m_algorithm_cache;

        // Managers
        std::unique_ptr<algorithms::ParallelComparisonManager> m_comparison_manager;
        algorithms::SideBySideVisualizer m_visualizer;

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
        int m_max_value{200};
        
        // Animation
        double m_animation_time{0.0};
    };

} // namespace c2l::scenes

#endif // CODE2LOGIC_ALGORITHM_COMPARISON_SCENE_HPP