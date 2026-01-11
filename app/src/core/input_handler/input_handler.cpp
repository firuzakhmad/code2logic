#include "input_handler.hpp"

namespace c2l::core
{
    InputHandler::InputHandler(GLFWWindow& window)
        : m_window(window)
    {}

    void InputHandler::update()
    {
        // Saving previous states
        std::swap(m_previous_key_state, m_current_key_state);
        std::swap(m_previous_mouse_state, m_current_mouse_state);

        m_previous_mouse_position = m_current_mouse_position;
        m_current_mouse_position  = m_window.get_cursor_position();
        
        // GLFW key codes below GLFW_KEY_SPACE are ignored.
        for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key)
        {
            m_current_key_state[static_cast<size_t>(key)] = m_window.is_key_pressed(key);
        }

        // Updating current mouse button states
        for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; ++button)
        {
            m_current_mouse_state[static_cast<size_t>(button)] = m_window.is_mouse_button_pressed(button);
        }
    }

    bool InputHandler::is_key_pressed(int key) const
    {
        if (key < 0 || key > GLFW_KEY_LAST) return false;
        return m_current_key_state[static_cast<size_t>(key)];
    }

    bool InputHandler::is_key_just_pressed(int key) const
    {
        if (key < 0 || key > GLFW_KEY_LAST) return false;
        return m_current_key_state[static_cast<size_t>(key)] &&
               !m_previous_key_state[static_cast<size_t>(key)];
    }

    bool InputHandler::is_key_just_released(int key) const
    {
        if (key < 0 || key > GLFW_KEY_LAST) return false;
        return !m_current_key_state[static_cast<size_t>(key)] &&
               m_previous_key_state[static_cast<size_t>(key)];
    }

    bool InputHandler::is_mouse_button_pressed(int button) const
    {
        if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) return false;
        return m_current_mouse_state[static_cast<size_t>(button)];
    }

    bool InputHandler::is_mouse_button_just_pressed(int button) const
    {
        if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) return false;
        return m_current_mouse_state[static_cast<size_t>(button)] &&
               !m_previous_mouse_state[static_cast<size_t>(button)];
    }

    bool InputHandler::is_mouse_button_just_released(int button) const
    {
        if (button < 0 || button > GLFW_MOUSE_BUTTON_LAST) return false;
        return !m_current_mouse_state[static_cast<size_t>(button)] &&
                m_previous_mouse_state[static_cast<size_t>(button)];
    }

    glm::dvec2 InputHandler::get_mouse_position() const
    {
        return m_window.get_cursor_position();
    }

    glm::dvec2 InputHandler::get_mouse_delta() const
    {
        return m_current_mouse_position - m_previous_mouse_position;
    }
}