#ifndef UI_COMPONENTS_DOCKING_LAYOUT_HPP
#define UI_COMPONENTS_DOCKING_LAYOUT_HPP

#include "ui/core/panel.hpp"

namespace c2l::ui::components
{
    class DockingLayout final : public core::Panel
    {
    public:
        DockingLayout();
        ~DockingLayout() override = default;

        void render(ImGuiWindowFlags flags = 0) override;

        void set_show_background(bool show) { m_show_background = show; }
        void set_dockspace_id(const std::string& id) { m_dockspace_id = id; }

    private:
        bool m_show_background      {true};
        std::string m_dockspace_id  {"MainDockSpace"};
    };

    } // namespace c2l::ui::components

#endif // UI_COMPONENTS_DOCKING_LAYOUT_HPP