#ifndef CODE2LOGIC_ALGORITHM_VISUALIZER_SCENE_HPP
#define CODE2LOGIC_ALGORITHM_VISUALIZER_SCENE_HPP

#include "scenes/base_scene.hpp"
#include "scenes/scene_manager.hpp"
#include "algorithms/i_simple_algorithm.hpp"
#include "algorithms/managers/algorithm_manager.hpp"

namespace c2l::scenes
{
    class AlgorithmVisualizerScene final : public BaseScene
    {
    public:
        AlgorithmVisualizerScene(graphics::Renderer& renderer,
                                 core::ThreadManager& thread_manager,
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
        void render_algorithm_selector();
        void render_algorithm_visualizer();
        void render_controls();
        void render_data_controls();
        void render_thread_info();
        void render_stats_panel();
        void render_code_panel();
        void render_algorithm_description();

        void render_comparison_analysis(const c2l::algorithms::AlgorithmStep& step);
        void render_performance_metrics();
        void render_metric_card(const std::string& title,
                                const std::string& value,
                                const ImVec4& color);


        static void draw_code_line(
            size_t line_number,
            const std::string& line,
            bool highlighted,
            const std::unordered_map<std::string, std::string>* vars);

        void setup_main_menu() override;

        std::unique_ptr<algorithms::AlgorithmManager> m_algorithm_manager;

        bool m_playback_controls_ready{false};
    };

}



#endif //CODE2LOGIC_ALGORITHM_VISUALIZER_SCENE_HPP