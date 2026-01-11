#ifndef GLFW_WINDOW_HPP
#define GLFW_WINDOW_HPP


#include "window_settings.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <functional>
#include <string_view>

#include "imgui.h"


namespace c2l::core
{
    /**
     * @class GLFWWindow
     * @brief RAII wrapper for GLFW window and OpenGL context management
     *
     * Handles window creation, lifecycle, input processing, and OpenGL context.
     * Provides a clean, modern interface for window management using GLFW.
     */
    class GLFWWindow
    {
    public:
        /**
         * @brief Construct a new GLFWWindow with specified settings
         * @param settings Configuration for the window and OpenGL context
         * @throws std::runtime_error if window or OpenGL context creation fails
         */
        explicit GLFWWindow(WindowSettings settings = {});
        ~GLFWWindow();

        // Non-copyable, movable
        GLFWWindow(const GLFWWindow&) = delete;
        GLFWWindow& operator=(const GLFWWindow&) = delete;
        GLFWWindow(GLFWWindow&&) noexcept;
        GLFWWindow& operator=(GLFWWindow&&) noexcept;

        void poll_events();
        void swap_buffers();

        void configure_glfw_hints() const;
        void create_window();
        void initialize_context();
        void setup_callbacks();
        void setup_graphics();

        /**
         * @brief Check if window should close
         * @return True if close requested or error occurred
         */
        [[nodiscard]] bool should_close() const;

        /**
         * @brief Request window to close
         * @param should_close Whether window should close
         */
        void set_should_close(bool should_close);

        // Window properties
        [[nodiscard]] glm::ivec2 get_size() const;
        [[nodiscard]] glm::ivec2 get_framebuffer_size() const;
        [[nodiscard]] std::string_view get_title() const;
        [[nodiscard]] GLFWwindow* get_native_handle() const;

        // Input state queries
        [[nodiscard]] bool is_key_pressed(int key) const;
        [[nodiscard]] bool is_mouse_button_pressed(int button) const;
        [[nodiscard]] glm::dvec2 get_cursor_position() const;
        [[nodiscard]]glm::dvec2 get_cursor_delta();

        // Window management
        void set_vsync(bool enabled);
        void set_title(const std::string& title);
        void set_size(glm::ivec2 size);

        // Event callbacks
        using KeyCallback = std::function<void(int key, int action)>;
        using MouseCallback = std::function<void(int button, int action)>;
        using CursorCallback = std::function<void(double x, double y)>;
        using ScrollCallback = std::function<void(double x_offset, double y_offset)>;
        using ResizeCallback = std::function<void(int width, int height)>;
        using CloseCallback = std::function<void()>;

        void set_key_callback(KeyCallback callback);
        void set_mouse_callback(MouseCallback callback);
        void set_cursor_callback(CursorCallback callback);
        void set_scroll_callback(ScrollCallback callback);
        void set_resize_callback(ResizeCallback callback);
        void set_close_callback(CloseCallback callback);

    private:
        void initialize();
        void shutdown() noexcept;

        // GLFW callback handlers
        static void handle_key_event(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void handle_mouse_button(GLFWwindow* window, int button, int action, int mods);
        static void handle_cursor_pos(GLFWwindow* window, double x_pos, double y_pos);
        static void handle_scroll(GLFWwindow* window, double x_offset, double y_offset);
        static void handle_framebuffer_size(GLFWwindow* window, int width, int height);
        static void handle_window_close(GLFWwindow* window);

        GLFWwindow* m_window                {nullptr};
        WindowSettings m_window_settings;
        ImVec2 m_framebuffer_size           {0, 0};

        // Callbacks
        KeyCallback m_key_callback;
        MouseCallback m_mouse_callback;
        CursorCallback m_cursor_callback;
        ScrollCallback m_scroll_callback;
        ResizeCallback m_resize_callback;
        CloseCallback m_close_callback;

        // Cursor state
        glm::dvec2 m_cursor_position        {0.0, 0.0};
        glm::dvec2 m_last_cursor_position   {0.0, 0.0};
        bool m_cursor_delta_updated         {false};
    };

} // namespace c2l::core

#endif // GLFW_WINDOW_HPP