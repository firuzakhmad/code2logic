#ifndef UI_CORE_PANEL_HPP
#define UI_CORE_PANEL_HPP

#include "i_ui_component.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <string>
#include <memory>
#include <functional>

namespace c2l::ui::core
{
	/**
	 * @brief Derived from the IUIComponent class for all UI panels providing common functionality
	 * 
	 * Features:
	 * - Visibility control
	 * - Consistent styling
	 * - Name management
	 * - RAII lifecycle
	 * - Event callback
	 */
	class Panel : public IUIComponent 
	{
	public:
		explicit Panel(std::string name, bool should_apply_default_style = true);
		virtual ~Panel() = default;

		// Non-copyable, movable
		Panel(const Panel&) = delete;
		Panel& operator=(const Panel&) = delete;
		Panel(Panel&&) = default;
		Panel& operator=(Panel&&) = default;


		// IUIComponent interface implementation
		void render(ImGuiWindowFlags flags = 0)	override;
		void update([[maybe_unused]] double dt) override {};

		// Accessors
	    [[nodiscard]] const std::string& get_name() const noexcept { return m_name; }
	    [[nodiscard]] bool is_visible() const noexcept { return m_visible; }
	    [[nodiscard]] bool wants_focus() const noexcept { return m_wants_focus; }

	    void set_visible(bool visible) noexcept;
	    void set_wants_focus(bool focus) noexcept { m_wants_focus = focus; }
	    void set_position(const ImVec2& pos, ImGuiCond cond = ImGuiCond_FirstUseEver);
    	void set_size(const ImVec2& size, ImGuiCond cond = ImGuiCond_FirstUseEver);

	    // Event callbacks
	    using VisibilityCallback = std::function<void(bool)>;
	    void set_visibility_callback(VisibilityCallback callback);

	protected:
		void apply_default_style() const;
		void set_window_properties(const ImVec2& pos, 
								   const ImVec2& size, 
								   ImGuiCond pos_cond, 
								   ImGuiCond size_cond);
		virtual void on_visibility_changed(bool visible);

		std::string m_name;
		bool m_visible				{true};
		bool m_wants_focus			{false};
		ImVec2 m_position			{0, 0};
	    ImVec2 m_size				{0, 0};
	    ImGuiCond m_position_cond	{ImGuiCond_FirstUseEver};
	    ImGuiCond m_size_cond		{ImGuiCond_FirstUseEver};

	private:
		VisibilityCallback m_visibility_callback;
	}; 

} // namespace c2l::ui::core

#endif // UI_CORE_PANEL_HPP