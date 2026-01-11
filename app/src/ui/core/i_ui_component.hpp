#ifndef UI_CORE_I_UI_COMPONENT_HPP
#define UI_CORE_I_UI_COMPONENT_HPP

#include <string>
#include <imgui.h>

namespace c2l::ui::core
{
	/**
	 * @brief Base interface for all UI components
	 * 
	 * Provides common functionality and polymorphism for all UI elements
	 */
	class IUIComponent
	{
	public:
		virtual ~IUIComponent() = default;

		virtual void render(ImGuiWindowFlags flags = 0)						= 0;
		virtual void update([[maybe_unused]] double dt) {}
		virtual void initialize() {}
	};

} // namespace c2l::ui::core

#endif // UI_CORE_I_UI_COMPONENT_HPP