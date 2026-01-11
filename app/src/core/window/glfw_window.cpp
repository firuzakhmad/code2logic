#include "glfw_window.hpp"
#include "core/utils/assert/assert.hpp"
#include "core/utils/logger/logger.hpp"

#include <utility>

#include "imgui.h"

namespace c2l::core
{
    GLFWWindow::GLFWWindow(WindowSettings window_settings)
        : m_window_settings{std::move(window_settings)}

    {
        initialize();
    }

    GLFWWindow::GLFWWindow(GLFWWindow&& other) noexcept
        : m_window{other.m_window},
          m_window_settings{std::move(other.m_window_settings)},
          m_key_callback{std::move(other.m_key_callback)},
          m_mouse_callback{std::move(other.m_mouse_callback)},
          m_cursor_callback{std::move(other.m_cursor_callback)},
          m_scroll_callback{std::move(other.m_scroll_callback)},
          m_resize_callback{std::move(other.m_resize_callback)},
          m_close_callback{std::move(other.m_close_callback)},
          m_cursor_position{other.m_cursor_position},
          m_last_cursor_position{other.m_last_cursor_position}
    {
        other.m_window = nullptr;
        if (m_window)
        {
            glfwSetWindowUserPointer(m_window, this);
        }
    }

    GLFWWindow& GLFWWindow::operator=(GLFWWindow&& other) noexcept
    {
        if (this != &other)
        {
            shutdown();

            m_window = other.m_window;
            m_window_settings = std::move(other.m_window_settings);
            m_key_callback = std::move(other.m_key_callback);
            m_mouse_callback = std::move(other.m_mouse_callback);
            m_cursor_callback = std::move(other.m_cursor_callback);
            m_scroll_callback = std::move(other.m_scroll_callback);
            m_resize_callback = std::move(other.m_resize_callback);
            m_close_callback = std::move(other.m_close_callback);
            m_cursor_position = other.m_cursor_position;
            m_last_cursor_position = other.m_last_cursor_position;

            other.m_window = nullptr;

            if (m_window)
            {
                glfwSetWindowUserPointer(m_window, this);
            }
        }
        return *this;
    }

    GLFWWindow::~GLFWWindow() { shutdown(); }

    void GLFWWindow::initialize()
    {
        if (!glfwInit())
        {
            LOG_FATAL("Failed to initialize GLFW!");
            return;
        }

        configure_glfw_hints();

        create_window();

        if (!m_window)
        {
            LOG_FATAL("Failed to create GLFW window");
            glfwTerminate();
            return;
        }

        initialize_context();
        setup_callbacks();
        setup_graphics();

        LOG_INFO("Window created successfully: {}x{} ({}x{} framebuffer)",
                 m_window_settings.size.x, m_window_settings.size.y,
                 m_framebuffer_size.x, m_framebuffer_size.y);
    }

    void GLFWWindow::configure_glfw_hints() const
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, m_window_settings.graphics.major_version);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, m_window_settings.graphics.minor_version);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);  // Always set for compatibility

        // Window properties
        glfwWindowHint(GLFW_RESIZABLE, m_window_settings.resizable);
        glfwWindowHint(GLFW_DECORATED, m_window_settings.decorated);
        glfwWindowHint(GLFW_VISIBLE, m_window_settings.visible);
        glfwWindowHint(GLFW_FOCUSED, m_window_settings.focused);
        glfwWindowHint(GLFW_MAXIMIZED, m_window_settings.maximized);
        glfwWindowHint(GLFW_FLOATING, m_window_settings.floating);
        glfwWindowHint(GLFW_SAMPLES, m_window_settings.graphics.msaa_samples);

    #ifdef __APPLE__
        glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, m_window_settings.platform.enable_high_dpi);
        glfwWindowHint(GLFW_COCOA_GRAPHICS_SWITCHING, GLFW_TRUE);
    #endif

        if (m_window_settings.platform.transparent_framebuffer || m_window_settings.opacity < 1.0f)
        {
            glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
        }
    }

    void GLFWWindow::create_window()
    {
        GLFWmonitor* monitor = nullptr;
        if (m_window_settings.fullscreen)
        {
            monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            m_window_settings.size = {mode->width, mode->height};
        }

        m_window = glfwCreateWindow(
            static_cast<int>(m_window_settings.size.x),
            static_cast<int>(m_window_settings.size.y),
            m_window_settings.title.c_str(),
            monitor,
            nullptr
        );
    }

    void GLFWWindow::initialize_context()
    {
        glfwMakeContextCurrent(m_window);
        glfwSetWindowUserPointer(m_window, this);

        // Loading OpenGL functions
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        {
            shutdown();
            LOG_FATAL("Failed to initialize GLAD!");
            throw std::runtime_error("Failed to initialize OpenGL context");
        }

        // Setting VSync
        glfwSwapInterval(m_window_settings.v_sync ? 1 : 0);

        glfwSetWindowOpacity(m_window, m_window_settings.opacity);
    }

    void GLFWWindow::setup_graphics()
    {
        int fb_width, fb_height;
        glfwGetFramebufferSize(m_window, &fb_width, &fb_height);
        m_framebuffer_size = {
            static_cast<float>(fb_width),
            static_cast<float>(fb_height)
        };

        glfwGetWindowSize(m_window,
            reinterpret_cast<int*>(&m_window_settings.size.x),
            reinterpret_cast<int*>(&m_window_settings.size.y));

        glViewport(0, 0, fb_width, fb_height);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (m_window_settings.graphics.msaa_samples > 0)
        {
            glEnable(GL_MULTISAMPLE);
        }
    }

    void GLFWWindow::setup_callbacks()
    {
        glfwSetKeyCallback(m_window, handle_key_event);
        glfwSetMouseButtonCallback(m_window, handle_mouse_button);
        glfwSetCursorPosCallback(m_window, handle_cursor_pos);
        glfwSetScrollCallback(m_window, handle_scroll);
        glfwSetFramebufferSizeCallback(m_window, handle_framebuffer_size);
        glfwSetWindowCloseCallback(m_window, handle_window_close);

        glfwGetCursorPos(m_window, &m_cursor_position.x, &m_cursor_position.y);
        m_last_cursor_position = m_cursor_position;
    }


    void GLFWWindow::shutdown() noexcept
    {
        if (m_window)
        {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
        glfwTerminate();
    }

    void GLFWWindow::poll_events()
    {
        glfwPollEvents();
        m_cursor_delta_updated = false;
    }

    void GLFWWindow::swap_buffers()
    {
        glfwSwapBuffers(m_window);
    }

    bool GLFWWindow::should_close() const
    {
        return glfwWindowShouldClose(m_window);
    }

    void GLFWWindow::set_should_close(bool should_close)
    {
        glfwSetWindowShouldClose(m_window, should_close ? GLFW_TRUE : GLFW_FALSE);
    }

    glm::ivec2 GLFWWindow::get_size() const
    {
        return m_window_settings.size;
    }

    glm::ivec2 GLFWWindow::get_framebuffer_size() const
    {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);
        return {width, height};
    }

    std::string_view GLFWWindow::get_title() const
    {
        return m_window_settings.title;
    }

    GLFWwindow* GLFWWindow::get_native_handle() const
    {
        return m_window;
    }

    bool GLFWWindow::is_key_pressed(int key) const
    {
        return glfwGetKey(m_window, key) == GLFW_PRESS;
    }

    bool GLFWWindow::is_mouse_button_pressed(int button) const
    {
        return glfwGetMouseButton(m_window, button) == GLFW_PRESS;
    }

    glm::dvec2 GLFWWindow::get_cursor_position() const
    {
        return m_cursor_position;
    }

    glm::dvec2 GLFWWindow::get_cursor_delta()
    {
        if (!m_cursor_delta_updated)
        {
            m_last_cursor_position = m_cursor_position;
            glfwGetCursorPos(m_window, &m_cursor_position.x, &m_cursor_position.y);
            m_cursor_delta_updated = true;
        }
        return m_cursor_position - m_last_cursor_position;
    }

    void GLFWWindow::set_vsync(bool enabled)
    {
        glfwSwapInterval(enabled ? 1 : 0);
        m_window_settings.v_sync = enabled;
    }

    void GLFWWindow::set_title(const std::string& title)
    {
        glfwSetWindowTitle(m_window, title.c_str());
        m_window_settings.title = title;
    }

    void GLFWWindow::set_size(glm::ivec2 size)
    {
        glfwSetWindowSize(m_window, size.x, size.y);
        m_window_settings.size = size;
    }

    void GLFWWindow::set_key_callback(KeyCallback callback)
    {
        m_key_callback = std::move(callback);
    }

    void GLFWWindow::set_mouse_callback(MouseCallback callback)
    {
        m_mouse_callback = std::move(callback);
    }

    void GLFWWindow::set_cursor_callback(CursorCallback callback)
    {
        m_cursor_callback = std::move(callback);
    }

    void GLFWWindow::set_scroll_callback(ScrollCallback callback)
    {
        m_scroll_callback = std::move(callback);
    }

    void GLFWWindow::set_resize_callback(ResizeCallback callback)
    {
        m_resize_callback = std::move(callback);
    }

    void GLFWWindow::set_close_callback(CloseCallback callback)
    {
        m_close_callback = std::move(callback);
    }

    // Static callback implementations
    void GLFWWindow::handle_key_event(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        if (self && self->m_key_callback)
        {
            self->m_key_callback(key, action);
        }
    }

    void GLFWWindow::handle_mouse_button(GLFWwindow* window, int button, int action, int mods)
    {
        auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        if (self && self->m_mouse_callback)
        {
            self->m_mouse_callback(button, action);
        }
    }

    void GLFWWindow::handle_cursor_pos(GLFWwindow* window, double xpos, double ypos)
    {
        auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        if (self)
        {
            self->m_cursor_position = {xpos, ypos};
            if (self->m_cursor_callback)
            {
                self->m_cursor_callback(xpos, ypos);
            }
        }
    }

    void GLFWWindow::handle_scroll(GLFWwindow* window, double xoffset, double yoffset)
    {
        auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        if (self && self->m_scroll_callback)
        {
            self->m_scroll_callback(xoffset, yoffset);
        }
    }

    void GLFWWindow::handle_framebuffer_size(GLFWwindow* window, int width, int height)
    {
        auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        if (self)
        {
            glViewport(0, 0, width, height);

            // Updating the window settings with the new framebuffer size
            self->m_window_settings.size.x = static_cast<unsigned int>(width);
            self->m_window_settings.size.y = static_cast<unsigned int>(height);

            if (self->m_resize_callback)
            {
                self->m_resize_callback(width, height);
            }
        }
    }

    void GLFWWindow::handle_window_close(GLFWwindow* window)
    {
        auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
        if (self && self->m_close_callback)
        {
            self->m_close_callback();
        }
    }


} // namespace c2l::core