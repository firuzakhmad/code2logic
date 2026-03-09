#include "scenes/algorithm_comparison_scene.hpp"

namespace c2l::scenes
{
    AlgorithmComparisonScene::AlgorithmComparisonScene(
        graphics::Renderer& renderer,
        core::ThreadManager& thread_manager,
        core::JsonConfigManager& json_config_manager,
        core::resources::ResourceManager& resource_manager,
        ui::managers::IconManager& icon_manager)
        : BaseScene{
            renderer, 
            thread_manager, 
            json_config_manager, 
            resource_manager, 
            icon_manager
        }
    {

    }

    void AlgorithmComparisonScene::on_create()
    {
        setup_ui_components();
    }
    void AlgorithmComparisonScene::on_destroy()
    {
        m_ui_manager->unregister_all_components();
    }
    void AlgorithmComparisonScene::on_activate()
    {
        if (!is_created())
        {
            on_create();
    	    mark_created();
        }
    }
    void AlgorithmComparisonScene::on_deactivate()
    {
    	m_ui_manager->hide_all_panels();
        m_ui_manager->close_all_popups();
        m_ui_manager->hide_all_widgets();
    }

    void AlgorithmComparisonScene::process_input(const core::InputHandler &input)
    {
        if (input.is_key_just_pressed(GLFW_KEY_ESCAPE))
        {
            request_scene_pop();
        }
    }

    void AlgorithmComparisonScene::update(double dt)
    {
        m_ui_manager->update(dt);
    }

    void AlgorithmComparisonScene::render()
    {
        m_renderer.render();

        m_ui_manager->render();

        ImGui::Begin("Algorithm Comparison");
        ImGui::Text("Comparison Running...");
        ImGui::End();

        render_common_ui();

        m_renderer.clear();
    }

} // namespace c2l::scenes
