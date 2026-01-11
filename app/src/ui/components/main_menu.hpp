#ifndef UI_COMPONENTS_MAIN_MENU_HPP
#define UI_COMPONENTS_MAIN_MENU_HPP

#include "ui/core/panel.hpp"

#include <functional>
#include <vector>
#include <string>

namespace c2l::ui::components
{
    class MainMenu final : public core::Panel
    {
    public:
        using MenuItemCallback = std::function<void()>;

        struct MenuItem
        {
            std::string label;
            MenuItemCallback callback;
            bool* toggle_state{nullptr};
            std::string shortcut;
        };

        struct Menu
        {
            std::string label;
            std::vector<MenuItem> items;
        };

        MainMenu();

        void render(ImGuiWindowFlags flags = 0) override;

        // Menu configuration
        void add_menu(const std::string& label, std::vector<MenuItem> items);
        void set_connection_status(bool connected, const std::string& status_text = "");

        // Status indicators
        void add_status_indicator(const std::string& text, const ImVec4& color);
        void clear_status_indicators();
    private:
        void render_menus();
        void render_status_bar();

        std::vector<Menu> m_menus;
        std::vector<std::pair<std::string, ImVec4>> m_status_indicators;
        bool m_connection_status        {false};
        std::string m_connection_text   {"DISCONNECTED"};

    };
    
} // namespace c2l::ui::components

#endif // UI_COMPONENTS_MAIN_MENU_HPP