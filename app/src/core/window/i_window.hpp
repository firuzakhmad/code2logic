#ifndef I_WINDOW_HPP
#define I_WINDOW_HPP

#include "core/window/window_settings.hpp"

#include <string>
#include <utility>
#include <functional>
#include <string_view>

namespace c2l::core
{
    /**
     * @class IWindow
     * @brief Abstract base class representing a platform-agnostic
     * window interface.
     * Provides core window functionality including event callbacks,
     * dimension management, and rendering synchronization.
     */
    class IWindow
    {
    public:
        /**
         * @brief Constructs a window with specified struct of `WindowSettings`
         * @param WindowSettings Configuration for the window
         */
        explicit IWindow(WindowSettings window_settings = {})
            : m_window_settings{std::move(window_settings)}
        {
            m_window_settings.validate();
        }

        virtual ~IWindow() = default;

        IWindow(const IWindow&) = delete;
        IWindow& operator=(const IWindow&) = delete;

        // Default move operations
        IWindow(IWindow&&) noexcept = default;
        IWindow& operator=(IWindow&&) noexcept = default;

        /**
         * @return Current window width in pixels
         */
        [[nodiscard]] unsigned int get_width() const noexcept
        {
            return m_window_settings.size.x;
        }

        /**
         * @return Current window height in pixels
         */
        [[nodiscard]] unsigned int get_height() const noexcept
        {
            return m_window_settings.size.y;
        }

        /**
         * @return Current window title
         */
        [[nodiscard]] const std::string_view& get_title() const noexcept
        {
            return m_window_settings.title;
        }

        // Event callback accessors
        [[nodiscard]] const std::function<void()>& get_on_exit() const noexcept
        {
            return m_on_exit;
        }

        [[nodiscard]] const std::function<void(int)>& get_on_key_down() const noexcept
        {
            return m_on_key_down;
        }

        [[nodiscard]] const std::function<void(const uint8_t*)>& get_on_late_keys_down() const noexcept
        {
            return m_on_late_keys_down;
        }

        [[nodiscard]] const std::function<void(int, int, int, int)>& get_on_mouse_move() const noexcept
        {
            return m_on_mouse_move;
        }

        [[nodiscard]] const std::function<void(int, int, int)>& get_on_mouse_down() const noexcept
        {
            return m_on_mouse_down;
        }

        // Setters
        void set_on_exit(std::function<void()> callback) noexcept
        {
            m_on_exit = std::move(callback);
        }

        void set_on_key_down(std::function<void(int)> callback) noexcept
        {
            m_on_key_down = std::move(callback);
        }

        void set_on_key_up(std::function<void(int)> callback) noexcept
        {
            m_on_key_up = std::move(callback);
        }

        void set_on_late_keys_down(std::function<void(const uint8_t*)> callback) noexcept
        {
            m_on_late_keys_down = std::move(callback);
        }

        void set_on_mouse_move(std::function<void(int, int, int, int)> callback) noexcept
        {
            m_on_mouse_move = std::move(callback);
        }

        void set_on_mouse_down(std::function<void(int, int, int)> callback) noexcept
        {
            m_on_mouse_down = std::move(callback);
        }

        /**
         * @brief Processes all pending window events
         * @note Must be called regularly to maintain window responsiveness
         */
        virtual void poll_events() = 0;

        /**
         * @brief Swaps the front and back buffers
         * @note Should be called after completing rendering for a frame
         */
        virtual void swap_buffers() const noexcept = 0;

        /**
         * @brief Checks if window should close
         * @return True if window close requested
         */
        [[nodiscard]] virtual bool should_close() const noexcept = 0;
    protected:
        WindowSettings m_window_settings;

        std::function<void()> m_on_exit;
        std::function<void(int)> m_on_key_down;
        std::function<void(int)> m_on_key_up;
        std::function<void(const uint8_t*)> m_on_late_keys_down;
        std::function<void(int, int, int, int)> m_on_mouse_move;
        std::function<void(int, int, int)> m_on_mouse_down;
    };

} // namespace c2l::core

#endif // I_WINDOW_HPP