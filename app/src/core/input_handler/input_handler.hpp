#ifndef INPUTHANDLER_HPP
#define INPUTHANDLER_HPP

#include "core/window/glfw_window.hpp"

#include <array>
#include <cstdint>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

namespace c2l::core
{
    /**
     * @class InputHandler
     * @brief Frame-based input state tracker using GLFW polling.
     *
     * InputHandler captures keyboard and mouse states once per frame and
     * provides deterministic queries for pressed, just pressed, and just released states.
     *
     * @note update() must be called exactly once per frame before querying input.
     * @note This class does not throw exceptions.
     */
    class InputHandler
    {
    public:
        explicit InputHandler(GLFWWindow& window);

        /**
         * @brief Update input state for the current frame
         * Should be called once per frame before processing input
         */
        void update();

        // Key state queries
        /**
         * @brief Check if a key is currently held down.
         *
         * @param key GLFW key code.
         * @return True if the key transitioned from released to pressed this frame.
         */
        [[nodiscard]] bool is_key_pressed(int key) const;
        [[nodiscard]] bool is_key_just_pressed(int key) const;
        [[nodiscard]] bool is_key_just_released(int key) const;

        // Mouse state queries
        [[nodiscard]] bool is_mouse_button_pressed(int button) const;
        [[nodiscard]] bool is_mouse_button_just_pressed(int button) const;
        [[nodiscard]] bool is_mouse_button_just_released(int button) const;
        [[nodiscard]] glm::dvec2 get_mouse_position() const;
        [[nodiscard]] glm::dvec2 get_mouse_delta() const;

    private:
        GLFWWindow& m_window;

        // Current frame state
        std::array<uint8_t, GLFW_KEY_LAST + 1> m_current_key_state{};
        std::array<uint8_t, GLFW_MOUSE_BUTTON_LAST + 1> m_current_mouse_state{};

        std::array<uint8_t, GLFW_KEY_LAST + 1> m_previous_key_state{};
        std::array<uint8_t, GLFW_MOUSE_BUTTON_LAST + 1> m_previous_mouse_state{};

        glm::dvec2 m_current_mouse_position     {0.0, 0.0};
        glm::dvec2 m_previous_mouse_position    {0.0, 0.0};
    };

} // namespace c2l::core

#endif // INPUTHANDLER_HPP