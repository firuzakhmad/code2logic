#ifndef IMGUI_MANAGER_HPP
#define IMGUI_MANAGER_HPP

#include <imgui.h>

namespace c2l::core
{
	class GLFWWindow;
}

namespace c2l::graphics
{
	/*
	 * @brief Manages ImGui context and rendering lifecycle
	 * 
	 * Handles initialization frame management, and rendering of ImGui
	*/
	class ImGuiManager {
	public:
		struct ThemeConfig
		{
			// Sizing
			float button_width				{100.0f};
			float button_height				{20.0f};
			float icon_button_width			{20.0f};
			float icon_button_size			{20.0f};
			float slider_width				{200.0f};
			float slider_height				{20.0f};
			float progressbar_width			{20.0f};
			float progressbar_height		{25.0f};

			// Colors
			ImVec4 primary					{0.2f, 0.6f, 1.0f, 1.0f};
			ImVec4 secondary				{0.3f, 0.8f, 0.3f, 1.0f};
			ImVec4 warning					{1.0f, 0.5f, 0.0f, 1.0f};

			ImVec4 button_color				{0.0f, 0.286f, 0.447f, 1.0f};
			ImVec4 button_hovered_color		{0.0f, 0.611f, 1.0f, 1.0f};
			ImVec4 button_active_color		{0.0f, 0.611f, 1.0f, 1.0f};

			// Spacing
			ImVec2 frame_padding			{4, 3};
			ImVec2 window_padding			{8, 8};
			ImVec2 item_spacing				{8, 4};
			ImVec2 item_inner_spacing		{4, 4};
			float padding_small				{4.0f};
			float padding_medium			{8.0f};
			float spacing_small				{4.0f};
			float spacing_medium			{8.0f};

			// Rounding
			float frame_rounding			{4.0f};
			float grab_rounding				{4.0f};
			float tab_rounding				{4.0f};

			static ThemeConfig dark_theme();
			static ThemeConfig light_theme();
			static ThemeConfig custom_theme();
		};

		/*
		 * @brief Construct a new ImGui Manager object
		 * @param window The window to attach ImGui to
		 */
		explicit ImGuiManager(core::GLFWWindow& window);
		~ImGuiManager();

		ImGuiManager(const ImGuiManager&) = delete;
		ImGuiManager(ImGuiManager&&) = delete;
		ImGuiManager& operator=(const ImGuiManager&) = delete;
		ImGuiManager& operator=(ImGuiManager&&) = delete;

	    void begin_frame() const;
	    void end_frame() const;

		void set_theme(const ThemeConfig& theme);
		void apply_theme() const;

		[[nodiscard]] const ThemeConfig& get_current_theme() const noexcept;
		[[nodiscard]] ThemeConfig& get_current_theme() noexcept;
		[[nodiscard]] ImVec2 get_button_size() const noexcept;
		[[nodiscard]] ImVec2 get_icon_button_size() const noexcept;

	private:
		void initialize();
	    void shutdown() noexcept;

		core::GLFWWindow& m_window;
	    bool m_initialized			{false};
		ThemeConfig m_current_theme	{};
	};
} // namespace c2l::graphics

#endif // IMGUI_MANAGER_HPP