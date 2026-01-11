#include "main_menu.hpp"
#include "core/utils/logger/logger.hpp"

namespace c2l::ui::components
{
	MainMenu::MainMenu()
		: core::Panel("MainMenu", false)
	{
		set_visible(true);
	}

	void MainMenu::render([[maybe_unused]] ImGuiWindowFlags flags)
	{
		if (!m_visible) return;

	    if (ImGui::BeginMainMenuBar())
	    {
	        render_menus();
	        render_status_bar();
	        ImGui::EndMainMenuBar();
	    }
	}

	void MainMenu::render_menus() 
	{
	    for (const auto& menu : m_menus) 
	    {
	        if (ImGui::BeginMenu(menu.label.c_str())) 
	        {
	            for (const auto& item : menu.items) 
	            {
	                if (ImGui::MenuItem(item.label.c_str(),
	                                   item.shortcut.empty() ? nullptr : item.shortcut.c_str(),
	                                   item.toggle_state)) {
	                    if (item.callback) 
	                    {
	                        item.callback();
	                    }
	                }
	            }
	            ImGui::EndMenu();
	        }
	    }
	}

	void MainMenu::render_status_bar() 
	{
	    if (m_status_indicators.empty()) return;

	    // Calculate total width needed for status indicators
	    float total_width = 0.0f;
	    for (const auto& [text, color] : m_status_indicators) 
	    {
	        total_width += ImGui::CalcTextSize(text.c_str()).x + 20.0f; // + padding
	    }

	    // Push items to the right
	    ImGui::SameLine(ImGui::GetWindowWidth() - total_width);

	    for (const auto& [text, color] : m_status_indicators) 
	    {
	        ImGui::PushStyleColor(ImGuiCol_Text, color);
	        ImGui::Text("%s", text.c_str());
	        ImGui::PopStyleColor();
	        ImGui::SameLine();
	    }
	}

	void MainMenu::add_menu(const std::string& label, std::vector<MenuItem> items) 
	{
	    m_menus.push_back({label, std::move(items)});
	}

	void MainMenu::set_connection_status(bool connected, const std::string& status_text) 
	{
	    m_connection_status = connected;
	    m_connection_text = status_text.empty() ? 
	        (connected ? "CONNECTED" : "DISCONNECTED") : status_text;
	    
	    // Update status indicator
	    clear_status_indicators();
	    add_status_indicator(
	        "● " + m_connection_text,
	        connected ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(1.0f, 0.0f, 0.0f, 1.0f)
	    );
	}

	void MainMenu::add_status_indicator(const std::string& text, const ImVec4& color) 
	{
	    m_status_indicators.emplace_back(text, color);
	}

	void MainMenu::clear_status_indicators() 
	{
	    m_status_indicators.clear();
	}


} // namespace c2l::ui::components