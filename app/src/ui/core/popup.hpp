#ifndef UI_CORE_POPUP_HPP
#define UI_CORE_POPUP_HPP 

#include "panel.hpp"

#include <functional>

namespace c2l::ui::core
{
	enum class PopupState
	{
		Closed,
		Opening,
		Open, 
		Closing
	};

	/**
	 * @brief Base class for all modal popup windows
	 * 
	 * Enhanced features:
	 * - State machine management
	 * - Animation support
	 * - Position/size control
	 * - Modal/non-modal options
	 * - Auto-close behavior
	 */
	class Popup : public Panel
	{
	public:
		explicit Popup(std::string name);
		~Popup() override = default;

		// Panel interface implementation
		void render(ImGuiWindowFlags flags = 0) override;
		void update(double dt) override;

		void open();
		void close();
		void toggle() { is_open() ? close() : open(); }

		[[nodiscard]] bool is_open() const noexcept { return m_state == PopupState::Open; }
		[[nodiscard]] bool is_close() const noexcept { return m_state == PopupState::Closed; }
		[[nodiscard]] PopupState get_state() const noexcept { return m_state; }
		[[nodiscard]] bool is_transitioning() const noexcept
		{
			return m_state == PopupState::Opening || m_state == PopupState::Closing; 
		} 

		void set_modal(bool modal) noexcept { m_modal = modal; }
		void set_auto_close(bool auto_close) noexcept { m_auto_close = auto_close; }
	    void set_center_on_open(bool center) noexcept { m_center_on_open = center; } 

	protected:
		virtual void on_open() {}
		virtual void on_close() {}
		virtual void render_content() = 0;
		
		// State tracking
		PopupState m_state			{PopupState::Closed};
		bool m_modal				{true};
		bool m_auto_close			{true};
		bool m_center_on_open		{true};

	private:
		void update_position();
		void handle_auto_close(double dt);
	}; 

} // namespace c2l::ui::core

#endif // UI_CORE_POPUP_HPP