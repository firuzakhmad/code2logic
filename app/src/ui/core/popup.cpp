#include "ui/core/popup.hpp"
#include "core/utils/logger/logger.hpp"

namespace c2l::ui::core
{
	Popup::Popup(std::string name)
		: Panel(std::move(name), false)
	{}
	
	void Popup::render(ImGuiWindowFlags flags)
	{
		if (!is_visible()) return;

		// Handling opening and closing
		if (m_state == PopupState::Opening)
		{
			ImGui::OpenPopup(get_name().c_str());
			m_state = PopupState::Open;
			update_position();
			on_open();
		}

		if (m_position.x != 0 || m_position.y != 0)
		{
			ImGui::SetNextWindowPos(m_position, m_position_cond);
		}

		set_window_properties(m_position, m_size, m_position_cond, m_size_cond);

    	// Combining with modal flag
	    ImGuiWindowFlags combined_flags = flags;
	    if (m_modal) 
	    {
	        combined_flags |= ImGuiWindowFlags_Modal;
	    }

	    // Rendering popup
	    if (ImGui::BeginPopup(get_name().c_str(), combined_flags))
	    {
	    	render_content();
	    	ImGui::EndPopup();
	    } else if (m_state == PopupState::Open)
	    {
	    	// Closing popup externally (clicked outside)
	    	m_state = PopupState::Closed;
	    	on_close();
	    }

	    // Handling explicit close requests
	    if (m_state == PopupState::Closing)
	    {
	    	ImGui::CloseCurrentPopup();
	    	m_state = PopupState::Closed;
	    	on_close();
	    }
	}

    void Popup::update([[maybe_unused]] double delta_time)
    {
    }

	void Popup::open()
	{
		if (m_state == PopupState::Closed)
		{
			m_state = PopupState::Opening;
			set_visible(true);
		}
	}

	void Popup::close()
	{
		if (m_state == PopupState::Open)
		{
			m_state = PopupState::Closing;
		}
	}

	void Popup::update_position() 
	{
	    if (m_center_on_open && (m_position.x == 0 && m_position.y == 0)) 
	    {
	        ImGuiViewport* viewport = ImGui::GetMainViewport();
	        m_position = ImVec2(
	            (viewport->Size.x - m_size.x) * 0.5f,
	            (viewport->Size.y - m_size.y) * 0.5f
	        );
	        m_position_cond = ImGuiCond_Appearing;
	    }
	}

	void Popup::handle_auto_close(double dt)
    {
        if (m_auto_close && is_open()) {
            m_state = PopupState::Closing;
        }
    }

} // namespace c2l::ui::core