#include "graphics/imgui_manager.hpp"
#include "core/utils/logger/logger.hpp"
#include "core/window/glfw_window.hpp"

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

namespace c2l::graphics
{
    ImGuiManager::ImGuiManager(core::GLFWWindow& window)
	    : m_window{window}
    {
	    initialize();
    }

    ImGuiManager::~ImGuiManager() { shutdown(); }

    void ImGuiManager::initialize()
    {
	    if (m_initialized) return;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        m_current_theme = ThemeConfig::dark_theme();
        apply_theme();

        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        if (!ImGui_ImplGlfw_InitForOpenGL(m_window.get_native_handle(), true))
        {
    	    LOG_FATAL("Failed to initialize ImGui GLFW backend");
        }

        if (!ImGui_ImplOpenGL3_Init("#version 410"))
        {
            ImGui_ImplGlfw_Shutdown();
            LOG_FATAL("Failed to initialize ImGui OpenGL backend");
        }

        m_initialized = true;
        LOG_INFO("ImGui initialized successfully");
    }

    void ImGuiManager::shutdown() noexcept
    {
	    if (!m_initialized) return;

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        m_initialized = false;
        LOG_INFO("ImGui shutdown completed");
    }

    void ImGuiManager::begin_frame() const
    {
	    if (!m_initialized) return;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiManager::end_frame() const
    {
	    if (!m_initialized) return;

        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            GLFWwindow* backup_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_context);
        }
    }

    void ImGuiManager::apply_theme() const
    {
        auto& style = ImGui::GetStyle();
        auto& colors = ImGui::GetStyle().Colors;

        // Applying sizes
        style.FramePadding = m_current_theme.frame_padding;
        style.WindowPadding = m_current_theme.window_padding;
        style.ItemSpacing = m_current_theme.item_spacing;
        style.ItemInnerSpacing = m_current_theme.item_inner_spacing;

        // Applying colors
        colors[ImGuiCol_Button] = m_current_theme.button_color;
        colors[ImGuiCol_ButtonHovered] = m_current_theme.button_hovered_color;
        colors[ImGuiCol_ButtonActive] = m_current_theme.button_active_color;

        // Applying rounding
        style.FrameRounding = m_current_theme.frame_rounding;
        style.GrabRounding = m_current_theme.grab_rounding;
        style.TabRounding = m_current_theme.tab_rounding;
    }

    void ImGuiManager::set_theme(const ThemeConfig &theme)
    {
        m_current_theme = theme;
        apply_theme();
    }

    ImGuiManager::ThemeConfig ImGuiManager::ThemeConfig::dark_theme()
    {
        // Todo
        return {};
    }

    ImGuiManager::ThemeConfig ImGuiManager::ThemeConfig::light_theme()
    {
        // Todo
        return {};
    }

    ImGuiManager::ThemeConfig ImGuiManager::ThemeConfig::custom_theme()
    {
        // Todo
        return {};
    }

    const ImGuiManager::ThemeConfig& ImGuiManager::get_current_theme() const noexcept
    {
        return m_current_theme;
    };

    ImGuiManager::ThemeConfig& ImGuiManager::get_current_theme() noexcept
    {
        return m_current_theme;
    };

    ImVec2 ImGuiManager::get_button_size() const noexcept
    {
        return {
            m_current_theme.button_width,
            m_current_theme.button_height
        };
    }

    ImVec2 ImGuiManager::get_icon_button_size() const noexcept {
        return {
            m_current_theme.icon_button_size,
            m_current_theme.icon_button_size
        };
    }

} // namespace c2l::graphics