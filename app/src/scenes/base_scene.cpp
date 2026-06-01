#include "base_scene.hpp"
#include "core/resources/resource_manager.hpp"
#include "core/file_system/std_file_system.hpp"

namespace c2l::scenes
{
	BaseScene::BaseScene(
		graphics::Renderer& renderer,
		core::ThreadManager& thread_manager,
		core::JsonConfigManager& json_config_manager,
		algorithms::AlgorithmRegistry& algorithm_registry,
		core::resources::ResourceManager& resource_manager,
		ui::managers::IconManager& icon_manager)
    : IScene(
		renderer, 
		thread_manager, 
		json_config_manager,
		algorithm_registry,
		resource_manager, 
		icon_manager)
    , m_ui_manager{std::make_unique<ui::managers::UIManager>()}
    {
	    setup_docking_layout();
		m_main_menu = m_ui_manager->register_component<
			ui::components::MainMenu
		>();
	}

	BaseScene::~BaseScene() {}

	void BaseScene::render_common_ui()
	{
		// Rendering ImGui demo if enabled
		if (m_show_demo_window)
		{
			ImGui::ShowDemoWindow(&m_show_demo_window);
		}
	}


	void BaseScene::set_navigation_callbacks(
		NavigationCallback push_callback,
		NavigationCallback switch_callback,
		ScenePopCallback pop_callback)
	{
		m_push_callback = std::move(push_callback);
		m_switch_callback = std::move(switch_callback);
		m_pop_callback = std::move(pop_callback);
		m_navigation_enabled = true;
	}

	void BaseScene::request_scene_push(
		const SceneType& scene_name)
	{
		if (m_navigation_enabled && m_push_callback)
		{
			m_push_callback(scene_name);
		}
		else
		{
			LOG_WARNING(
				"Navigation not available for scene switch: {}",
				SceneManager::scene_type_to_string(scene_name)
			);
		}
	}

	void BaseScene::request_scene_switch(
		const SceneType&  scene_name)
	{
		if (m_navigation_enabled && m_switch_callback)
		{
			m_switch_callback(scene_name);
		}
		else
		{
			LOG_WARNING(
				"Navigation not available for scene switch: {}",
				SceneManager::scene_type_to_string(scene_name)
			);
		}
	}

	void BaseScene::request_scene_pop()
	{

		if (m_navigation_enabled && m_pop_callback)
		{
			m_pop_callback();
		}
		else
		{
			LOG_WARNING("Navigation not available for scene pop");
		}
	}

	bool BaseScene::can_navigate() const
	{
		return m_navigation_enabled;
	}

} // namespace c2l::scenes
