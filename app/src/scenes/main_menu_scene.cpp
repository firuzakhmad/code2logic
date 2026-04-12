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

        ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);

        ImGui::Begin(
            "MainMenu",
            nullptr,
            ImGuiWindowFlags_NoDecoration
        );

        ImVec2 window_size = ImGui::GetWindowSize();

        float button_width = window_size.x * 0.25f;
        float button_height = 45.0f;
        float spacing = 15.0f;

        float total_height = (button_height * 3) + (spacing * 2);

        ImGui::SetCursorPosY((window_size.y - total_height) * 0.5f);

        auto center_button = [&](const char* label)
        {
            float cursor_x = (window_size.x - button_width) * 0.5f;
            ImGui::SetCursorPosX(cursor_x);
            return ImGui::Button(label, ImVec2(button_width, button_height));
        };

        if (center_button("Algorithm Visualizer"))
        {
            request_scene_push(SceneType::ALGORITHM_VISUALIZER);
        }

        ImGui::Dummy(ImVec2(0, spacing));

        if (center_button("Algorithm Comparison"))
        {
            request_scene_push(SceneType::ALGORITHM_COMPARISON);
        }

        ImGui::Dummy(ImVec2(0, spacing));

        if (center_button("Exit"))
        {
            m_renderer.get_window().set_should_close(true);
        }

        ImGui::End();

        render_common_ui();
        m_renderer.clear();
    }

} // namespace c2l::scenes
