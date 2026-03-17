#include "main_menu_scene.hpp"

#include <iostream>

namespace c2l::scenes
{
    MainMenuScene::MainMenuScene(
        graphics::Renderer& renderer,
        core::ThreadManager& thread_manager,
        core::JsonConfigManager& json_config_manager,
        algorithms::AlgorithmRegistry& algorithm_registry,
        core::resources::ResourceManager& resource_manager,
        ui::managers::IconManager& icon_manager
    )
        : BaseScene{
            renderer,
            thread_manager,
            json_config_manager,
            algorithm_registry,
            resource_manager,
            icon_manager
        }
    {

    }

    void MainMenuScene::on_create()
    {
        setup_ui_components();
    }
    void MainMenuScene::on_destroy()
    {
        m_ui_manager->unregister_all_components();
    }
    void MainMenuScene::on_activate()
    {
        if (!is_created())
        {
            on_create();
        	mark_created();
        }
    }
    void MainMenuScene::on_deactivate()
    {
    	m_ui_manager->hide_all_panels();
        m_ui_manager->close_all_popups();
        m_ui_manager->hide_all_widgets();
    }

    void MainMenuScene::process_input(
        const core::InputHandler &input)
    {
        if (input.is_key_just_pressed(GLFW_KEY_BACKSPACE))
        {
            request_scene_pop();
        }
    }

    void MainMenuScene::update(double dt)
    {
        m_ui_manager->update(dt);
    }

    void MainMenuScene::render()
    {
        m_renderer.render();

        m_ui_manager->render();

        ImGui::Begin("Scene Navigation Test");


        if (ImGui::Button("Algorithm Visualizer", ImVec2(250, 40)))
        {
            request_scene_push(SceneType::ALGORITHM_VISUALIZER);
        }

        if (ImGui::Button("Algorithm Comparison", ImVec2(250, 40)))
        {
            request_scene_push(SceneType::ALGORITHM_COMPARISON);
        }

        if (ImGui::Button("Exist", ImVec2(250, 40)))
        {
            m_renderer.get_window().set_should_close(true);
        }

        ImGui::End();

        render_common_ui();

        m_renderer.clear();
    }

} // namespace c2l::scenes
