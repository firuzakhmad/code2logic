#ifndef CODE2LOGIC_ALGORITHM_VISUALIZER_SCENE_HPP
#define CODE2LOGIC_ALGORITHM_VISUALIZER_SCENE_HPP

#include "scenes/base_scene.hpp"
#include "scenes/scene_manager.hpp"
#include "algorithms/core/i_simple_algorithm.hpp"
#include "algorithms/managers/algorithm_manager.hpp"
#include "core/json_config_manager/json_config_manager.hpp"
#include "algorithms/core/algorithm_registry.hpp"

#include  <unordered_map>
#include <string>

namespace c2l::scenes
{
    /**
     * @brief Scene for algorithm visualization with full JSON metadata support
     * 
     * This scene renders algorithm visualizations, metadata, pseudocode,
     * and controls using JSON-driven configurations.
     */
    class AlgorithmVisualizerScene final : public BaseScene
    {
    public:
        AlgorithmVisualizerScene(
            graphics::Renderer& renderer,
            core::ThreadManager& thread_manager,
            core::JsonConfigManager& json_config_manager,
            algorithms::AlgorithmRegistry& algorithm_registry,
            core::resources::ResourceManager& resource_manager,
            ui::managers::IconManager& icon_manager);
        ~AlgorithmVisualizerScene() override = default;

        // IScene interface
        /**
         * @copydoc IScene::on_create()
         */
        void on_create() override;

        /**
         * @copydoc IScene::on_destroy()
         */
        void on_destroy() override;

        /**
         * @copydoc IScene::on_activate()
         */
        void on_activate() override;

        /**
         * @copydoc IScene::on_deactivate()
         */
        void on_deactivate() override;

        /**
         * @copydoc IScene::process_input()
         */
        void process_input(const core::InputHandler& input) override;

        /**
         * @copydoc IScene::update()
         */
        void update(double dt) override;

        /**
         * @copydoc IScene::render()
         */
        void render() override;

    private: 
        // Rendering methods
        void render_algorithm_selector_panel();
        void render_algorithm_visualization_panel();
        void render_algorithm_control_panel();
        void render_algorithm_data_control_panel();
        void render_algorithm_code_panel();
        void render_algorithm_description_panel();
        void render_algorithm_stats_panel();
        void render_thread_info_panel();
        void render_algorithm_variable_inspector_panel();

        // Panel rendering helpers
        void render_algorithm_header(
            const algorithms::IAlgorithmMetadata* metadata
        );
        void render_complexity_badges(
            const algorithms::AlgorithmComplexityInfo& complexity
        );
        void render_properties_table(
            const algorithms::AlgorithmPropertiesInfo& properties
        );

        void render_code_line(
            size_t line_number,
            const std::string& line,
            bool highlight,
            const std::unordered_map<std::string, std::string>* vars = nullptr
        );

        void setup_main_menu() override;
        void setup_shortcuts_tooltip();

        std::unique_ptr<algorithms::AlgorithmManager> m_algorithm_manager;

        // Panel visibility flags
        bool m_show_algorithm_selector_panel            {true};
        bool m_show_algorithm_visualization_panel       {true};
        bool m_show_algorithm_control_panel             {true};
        bool m_show_algorithm_data_control_panel        {false};
        bool m_show_algorithm_code_panel                {true};
        bool m_show_algorithm_description_panel         {false};
        bool m_show_algorithm_stats_panel               {false};
        bool m_show_algorithm_performance_panel         {false};
        bool m_show_algorithm_variable_inspector_panel  {false};
        bool m_show_thread_info_panel                   {false};
        
        // UI state
        bool m_playback_controls_ready                  {false};
        float m_ui_scale                                {1.0f};

        // Cached data
        std::unordered_map<
            std::string,
            std::vector<const algorithms::AlgorithmInfo*>
        > m_cached_categorized_algorithms;
    };

}



#endif //CODE2LOGIC_ALGORITHM_VISUALIZER_SCENE_HPP