#ifndef UI_MANAGERS_UI_MANAGER_HPP
#define UI_MANAGERS_UI_MANAGER_HPP

#include "ui/core/panel.hpp"
#include "ui/core/popup.hpp"
#include "ui/core/widget.hpp"
#include "ui/core/i_ui_component.hpp"
#include "core/utils/logger/logger.hpp"

#include <memory>
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <algorithm>

#include "ui/components/docking_layout.hpp"

namespace c2l::ui::managers
{
	/**
	 * @brief Central manager for all UI panels and layout
	 */
	class UIManager
	{
	public:
		UIManager();
		~UIManager();

		void render();
		void update(double delta_time);

		void set_docking_layout(bool show) const;
    	// Component registration
	    template<typename T, typename... Args>
	    std::shared_ptr<T> register_component(Args&&... args) {
	        static_assert(std::is_base_of_v<core::Panel, T> || 
	                     std::is_base_of_v<core::Popup, T> ||
	                     std::is_base_of_v<core::Widget, T>,
	                     "T must derive from Panel, Popup, or Widget");
	        
	        auto component = std::make_shared<T>(std::forward<Args>(args)...);
	        return register_component_impl(component);
	    }

	    // Component retrieval by type
	    template<typename T>
	    std::shared_ptr<T> get_component() const {
	        static_assert(std::is_base_of_v<core::Panel, T> || 
	                     std::is_base_of_v<core::Popup, T> ||
	                     std::is_base_of_v<core::Widget, T>,
	                     "T must derive from Panel, Popup, or Widget");
	                     
	        for (const auto& [name, component] : m_component_map) {
	            if (auto derived = std::dynamic_pointer_cast<T>(component)) {
	                return derived;
	            }
	        }
	        return nullptr;
	    }

		// Retrieval by name
		[[nodiscard]] std::shared_ptr<core::Panel> get_panel(const std::string& name) const;
		[[nodiscard]] std::shared_ptr<core::Popup> get_popup(const std::string& name) const;
    	[[nodiscard]] std::shared_ptr<core::Widget> get_widget(const std::string& name) const;

		[[nodiscard]] const auto& get_all_panels() const noexcept { return m_panels; }
    	[[nodiscard]] const auto& get_all_popups() const noexcept { return m_popups; }
		[[nodiscard]] const auto& get_all_widgets() const noexcept { return m_widgets; }

		// Bulk operations
		void show_all_panels();
		void hide_all_panels();
		void close_all_popups();
		void hide_all_widgets();
    	void unregister_all_components();
		
		// Layout operations
		void bring_to_front(const std::string& panel_name);
		void send_to_back(const std::string& panel_name);

	private:
		template<typename T>
	    std::shared_ptr<T> register_component_impl(std::shared_ptr<T> component) 
	    {
	        const std::string& name = component->get_name();
	        
	        if (m_component_map.find(name) != m_component_map.end()) 
	        {
	            LOG_ERROR("UI component '{}' already registered", name);
	            throw std::runtime_error("Duplicate UI component: " + name);
	        }
	        
	        // Store in appropriate container
	        if constexpr (std::is_base_of_v<core::Panel, T>) 
	        {
	            m_panels.push_back(component);
	        } else if constexpr (std::is_base_of_v<core::Popup, T>) 
	        {
	            m_popups.push_back(component);
	        } else if constexpr (std::is_base_of_v<core::Widget, T>) 
	        {
	            m_widgets.push_back(component);
	        }
	        
	        m_component_map[name] = component;
	        return component;
	    }		 	

		std::unique_ptr<components::DockingLayout> m_docking_layout;

		std::vector<std::shared_ptr<core::Panel>> m_panels;
		std::vector<std::shared_ptr<core::Popup>> m_popups;
		std::vector<std::shared_ptr<core::Widget>> m_widgets;
    	std::unordered_map<std::string, std::shared_ptr<core::IUIComponent>> m_component_map;
	}; 

} // namespace c2l::ui::managers

#endif // UI_MANAGERS_UI_MANAGER_HPP
