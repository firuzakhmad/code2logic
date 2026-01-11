#include "panel.hpp"
#include "core/utils/logger/logger.hpp"

namespace c2l::ui::core
{
	Panel::Panel(std::string name, bool should_apply_default_style)
		: m_name{std::move(name)}
	{
		if (should_apply_default_style)
		{
			apply_default_style();
		}
	}

	void Panel::apply_default_style() const 
	{
	    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
	    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
	}

	void Panel::on_visibility_changed(bool visible) 
	{
	    if (m_visibility_callback) 
	    {
	        m_visibility_callback(visible);
	    }
	}

	void Panel::set_visible(bool visible) noexcept
    {
        if (m_visible != visible)
        {
            m_visible = visible;
            on_visibility_changed(visible);
        }
    }

	void Panel::set_position(const ImVec2& pos, ImGuiCond cond)
	{
		m_position 		= pos;
		m_position_cond = cond;
	}
	
	void Panel::set_size(const ImVec2& size, ImGuiCond cond)
	{
		m_size 		= size;
		m_size_cond = cond;
	}

	void Panel::set_visibility_callback(VisibilityCallback callback)
    {
        m_visibility_callback = std::move(callback);
    }

    void Panel::render([[maybe_unused]] ImGuiWindowFlags flags)
    {
        if (!is_visible()) return;

        set_window_properties(m_position, m_size, m_position_cond, m_size_cond);
    }

    void Panel::set_window_properties(const ImVec2& pos, const ImVec2& size, ImGuiCond pos_cond, ImGuiCond size_cond)
    {
        if (pos.x != 0 || pos.y != 0) {
            ImGui::SetNextWindowPos(pos, pos_cond);
        }
        if (size.x != 0 || size.y != 0) {
            ImGui::SetNextWindowSize(size, size_cond);
        }
    }

	
} // namespace c2l::ui::core
