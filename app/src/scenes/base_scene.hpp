#ifndef SCENES_BASE_SCENE_HPP
#define SCENES_BASE_SCENE_HPP

#include "core/resources/resource_manager.hpp"
#include "scenes/i_scene.hpp"
#include "ui/components/main_menu.hpp"
#include "ui/managers/icon_manager.hpp"
#include "scenes/scene_manager.hpp"
#include "algorithms/algorithm_types.hpp"

namespace c2l::scenes
{
	/**
	 * @class BaseScene
	 * @brief Convenience base class for scenes with UI and navigation support.
	 *
	 * BaseScene implements common functionality such as:
	 * - UI manager ownership
	 * - Scene navigation requests
	 * - Shared UI rendering helpers
	 *
	 * Derived scenes may override UI setup and rendering behavior.
	 *
	 * @note BaseScene does NOT own navigation logic; it only forwards requests.
	 */

	class BaseScene : public IScene
	{
	public:
		/**
		 * @brief Requests a scene change.
		 *
		 * Scenes may request navigation, but SceneManager
		 * remains the sole authority that performs transitions.
		 */
		using NavigationCallback = std::function<void(const scenes::SceneType& scene_type)>;
		using ScenePopCallback = std::function<void()>;

		BaseScene(graphics::Renderer& renderer,
				  core::ThreadManager& thread_manager,
				  core::JsonConfigManager& json_config_manager,
				  core::resources::ResourceManager& resource_manager,
				  ui::managers::IconManager& icon_manager);
		virtual ~BaseScene() override;

		void set_navigation_callbacks(
			NavigationCallback push_callback,
			NavigationCallback switch_callback,
			ScenePopCallback pop_callback);

		void request_scene_push(const SceneType&  scene_name);
		void request_scene_switch(const SceneType&  scene_name);
		void request_scene_pop();

		[[nodiscard]] bool can_navigate() const;

		// UI management
	    [[nodiscard]] ui::managers::UIManager& get_ui_manager() override 
		{ 
			return *m_ui_manager; 
		}
	    [[nodiscard]] const ui::managers::UIManager& get_ui_manager() const override 
		{ 
			return *m_ui_manager; 
		}	
	    [[nodiscard]] ui::components::MainMenu& get_main_menu() const 
		{ 
			return *m_main_menu; 
		}

	protected: 
		// Common UI setup that derived scenes can override
	    virtual void setup_ui_components() {}
	    virtual void setup_main_menu() {}
	    virtual void setup_docking_layout() {}
	    virtual void render_common_ui();

		bool render_icon_button(
			const std::string& id,
			ui::managers::IconType type,
			const std::function<void()>& callback = nullptr,
			const ImVec2& size = ImVec2(0.f, 0.f),
			bool enabled = true,
			const std::string& tooltip = "");

	    std::unique_ptr<ui::managers::UIManager> m_ui_manager;

	    std::shared_ptr<ui::components::MainMenu> m_main_menu;

		NavigationCallback m_push_callback;
		NavigationCallback m_switch_callback;
		ScenePopCallback m_pop_callback;

		std::unordered_map<std::string_view, std::vector<const algorithms::AlgorithmInfo*>> m_available_categorized_algorithms;

	private:
		bool m_show_demo_window				{false};
		bool m_navigation_enabled			{false};
	};

} // namespace c2l::scenes

#endif // SCENES_BASE_SCENE_HPP