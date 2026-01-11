#include "docking_layout.hpp"

namespace c2l::ui::components {

DockingLayout::DockingLayout() 
    : Panel("Docking Layout", false)
    {}

void DockingLayout::render([[maybe_unused]] ImGuiWindowFlags flags)
{
    if (!is_visible()) return;

    // Use ImGui's recommended approach for main window dockspace
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    // Set position and size to cover the entire viewport
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    // Important: Use WorkPos and WorkSize which already account for menu bars
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    if (m_show_background) {
        window_flags |= ImGuiWindowFlags_NoBackground;
    }

    ImGui::Begin("DockSpace Window", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    // Submitting the dockspace
    ImGuiID dockspace_id = ImGui::GetID(m_dockspace_id.c_str());
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    ImGui::End();
}

} // namespace c2l::ui::components