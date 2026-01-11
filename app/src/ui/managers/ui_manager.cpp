#include "ui_manager.hpp"

namespace c2l::ui::managers
{
	UIManager::UIManager()
		: m_docking_layout{std::make_unique<components::DockingLayout>()}
	{}

	UIManager::~UIManager()
	{
		unregister_all_components();
	}

	void UIManager::unregister_all_components() 
	{
	    m_panels.clear();
	    m_popups.clear();
	    m_widgets.clear();
	    m_component_map.clear();
	}

	void UIManager::render()
	{
		// Docking layout should be rendered first
		if (m_docking_layout && m_docking_layout->is_visible())
		{
			m_docking_layout->render();
		}

		// Render scene-specific UI components
        for (const auto& panel : m_panels) {
            if (panel->is_visible()) {
                panel->render();
            }
        }

        // Render popups
        for (const auto& popup : m_popups) {
            popup->render();
        }
	}

	void UIManager::update(double delta_time)
	{
		for (const auto& panel : m_panels)
		{
			panel->update(delta_time);
		}

		for (const auto& popup : m_popups)
		{
			popup->update(delta_time);
		}

		for (const auto& widget : m_widgets)
		{
			widget->update(delta_time);
		}
	}

	void UIManager::set_docking_layout(bool show) const
	{
		if (m_docking_layout) {
			m_docking_layout->set_show_background(true);
		}
		m_docking_layout->set_show_background(show);
	}


	std::shared_ptr<core::Panel> UIManager::get_panel(const std::string& name) const
	{
		auto it = m_component_map.find(name);
	    if (it != m_component_map.end()) {
	        return std::dynamic_pointer_cast<core::Panel>(it->second);
	    }
	    return nullptr;
	}
	std::shared_ptr<core::Popup> UIManager::get_popup(const std::string& name) const
	{
		auto it = m_component_map.find(name);
	    if (it != m_component_map.end()) {
	        return std::dynamic_pointer_cast<core::Popup>(it->second);
	    }
	    return nullptr;
	}

	std::shared_ptr<core::Widget> UIManager::get_widget(const std::string& name) const 
	{
	    auto it = m_component_map.find(name);
	    if (it != m_component_map.end()) {
	        return std::dynamic_pointer_cast<core::Widget>(it->second);
	    }
	    return nullptr;
	}

	// Bulk operations
	void UIManager::show_all_panels()
	{
		for (auto& panel : m_panels)
		{
			panel->set_visible(true);
		}
	}

	void UIManager::hide_all_panels()
	{
		for (auto& panel : m_panels)
		{
			panel->set_visible(false);
		}
	}

	void UIManager::close_all_popups()
	{
		for (auto& popup : m_popups)
		{
			if (popup->is_open())
			{
				popup->close();
			}
		}
	}

	void UIManager::hide_all_widgets() 
	{
	    for (auto& widget : m_widgets) {
	        widget->set_visible(false);
	    }
	}

	void UIManager::bring_to_front(const std::string& panel_name) 
	{
		auto panel = get_panel(panel_name);
		if (!panel) return;
		
		// Move to end of vector (rendered last = on top)
		auto it = std::find(m_panels.begin(), m_panels.end(), panel);
		if (it != m_panels.end()) 
		{
			std::rotate(it, it + 1, m_panels.end());
		}
	}

	void UIManager::send_to_back(const std::string& panel_name) {
		auto panel = get_panel(panel_name);
		if (!panel) return;
		
		// Move to beginning of vector (rendered first = on bottom)
		auto it = std::find(m_panels.begin(), m_panels.end(), panel);
		if (it != m_panels.end()) 
		{
			std::rotate(m_panels.begin(), it, it + 1);
		}
	}

} // namespace c2l::ui::managers
