#include "base_scene.hpp"
#include "core/resources/resource_manager.hpp"
#include "core/file_system/std_file_system.hpp"

namespace c2l::scenes
{
	BaseScene::BaseScene(
		graphics::Renderer& renderer,
		core::ThreadManager& thread_manager,
		core::JsonConfigManager& json_config_manager,
		core::resources::ResourceManager& resource_manager,
		ui::managers::IconManager& icon_manager)
    : IScene(
		renderer, 
		thread_manager, 
		json_config_manager, 
		resource_manager, 
		icon_manager)
    , m_ui_manager{std::make_unique<ui::managers::UIManager>()}
    {
	    setup_docking_layout();
		m_main_menu = m_ui_manager->register_component<ui::components::MainMenu>();
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

	bool BaseScene::render_icon_button(
		const std::string& id,
		ui::managers::IconType type,
		const std::function<void()>& callback,
		const ImVec2& size,
		bool enabled,
		const std::string& tooltip)
	{
		if (!ImGui::GetCurrentContext()) 
		{
			LOG_ERROR(
				"No ImGui context for icon button {}", 
				id
			);
			return false;
		}

		bool clicked = m_icon_manager.render_icon_button(
			id.c_str(),
			type,
			size,
			enabled ? ImVec4(1, 1, 1, 1) : ImVec4(0.5f, 0.5f, 0.5f, 0.5f),
			tooltip.empty() ? nullptr : tooltip.c_str());

		if (clicked && callback)
		{
			callback();
		}

		return clicked;
	}

} // namespace c2l::scenes
