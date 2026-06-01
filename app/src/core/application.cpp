#include "application.hpp"
#include "core/utils/logger/logger.hpp"
#include "core/utils/timer/timer.hpp"
#include "scenes/main_menu_scene.hpp"
#include "scenes/algorithm_visualizer_scene.hpp"
#include "core/resources/font_loader.hpp"
#include "scenes/algorithm_comparison_scene.hpp"

namespace c2l
{
    Application::Application()
    {
        LOG_INFO("Application initializing...");
        initialize_services();
    }

    void Application::initialize_services()
    {
        m_timer = std::make_unique<core::Timer>();
        m_timer->reset();

        m_window = std::make_unique<core::GLFWWindow>();
        m_input_handler = std::make_unique<core::InputHandler>(*m_window);
        m_renderer = std::make_unique<graphics::Renderer>(*m_window);

        m_thread_manager = std::make_unique<core::ThreadManager>();
        m_file_system = std::make_unique<core::filesystem::StdFileSystem>();
        m_json_config_manager = std::make_unique<core::JsonConfigManager>(
            *m_file_system,
            *m_thread_manager
        );
        m_json_config_manager->initialize();



        m_resource_manager = std::make_unique<core::resources::ResourceManager>(
            *m_file_system,
            *m_thread_manager
        );
        m_resource_manager->set_memory_budget(64 * 1024 * 1024);
        m_resource_manager->enable_hot_reloading(false);  // Disabling for debugging
        const auto preload_list = m_json_config_manager->get_preload_resources();
        m_resource_manager->preload_resources(preload_list);

        m_icon_manager = std::make_unique<c2l::ui::managers::IconManager>(
        *m_thread_manager,
        *m_resource_manager,
        *m_json_config_manager
        );

        m_algorithm_registry = std::make_unique<algorithms::AlgorithmRegistry>(
            *m_json_config_manager,
            *m_icon_manager
        );

        m_scene_manager = std::make_unique<scenes::SceneManager>(
        *m_renderer,
        *m_thread_manager,
        *m_json_config_manager,
        *m_algorithm_registry,
        *m_resource_manager,
        *m_icon_manager
        );

        LOG_INFO("Application initialization complete");
    }

    Application::~Application()
    {
        LOG_INFO("Application shutdown initiated...");

        m_scene_manager.reset();
        m_algorithm_registry.reset();
        m_icon_manager.reset();
        m_resource_manager.reset();
        m_json_config_manager.reset();
        m_file_system.reset();
        m_thread_manager.reset();
        m_renderer.reset();
        m_input_handler.reset();
        m_window.reset();
        m_timer.reset();

        LOG_INFO("Application destroyed.");
    }

    void Application::run()
    {
        constexpr double fixed_timestep = 1.0 / 60.0;
        constexpr double max_accumulated = 0.25;

        while (!m_window->should_close())
        {
            m_timer->update();

            if (m_timer->get_accumulated_time() > max_accumulated)
            {
                m_timer->consume_accumulated_time(
                    m_timer->get_accumulated_time() - max_accumulated
                );
            }

            m_window->poll_events();
            m_scene_manager->process_input(*m_input_handler);

            while (m_timer->get_accumulated_time() >= fixed_timestep)
            {
                m_scene_manager->update(fixed_timestep);
                m_icon_manager->update();
                m_timer->consume_accumulated_time(fixed_timestep);
            }

            m_scene_manager->render();

            m_window->swap_buffers();
            m_input_handler->update();
        }
    }

} // namespace c2l
