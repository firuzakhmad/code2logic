#ifndef WINDOW_SETTINGS_HPP
#define WINDOW_SETTINGS_HPP

#include <glm/glm.hpp>
#include <string>
#include <stdexcept>

namespace c2l::core
{
    /**
    * @struct WindowSettings
    * @brief Comprehensive configuration for window creation
    *
    * Contains all parameters needed to create and configure a window,
    * with built-in validation for parameter sanity.
    */
    struct WindowSettings
    {
        std::string title   {"Code2Logic"};
        glm::uvec2 size     {800, 600};
        bool v_sync         {true};
        bool resizable      {true};
        bool fullscreen     {false};
        bool decorated      {true};
        bool visible        {true};
        bool focused        {true};
        bool maximized      {false};
        bool floating       {false};

        // Graphics settings
        struct {
            int major_version   {3};
            int minor_version   {3};
            int msaa_samples    {4};
        } graphics;

        struct {
            bool enable_high_dpi            {true};
            bool transparent_framebuffer    {false};
        } platform;

        float opacity {1.0f};

        /**
        * @brief Validates all window settings
        * @throws std::invalid_argument if any setting is invalid
        */
        void validate() const
        {
            if (title.empty())
            {
                throw std::invalid_argument("Window title cannot be empty");
            }
            if (size.x == 0 || size.y == 0)
            {
                throw std::invalid_argument("Window size cannot be zero");
            }
            if (graphics.major_version < 3)
            {
                throw std::invalid_argument("OpenGL 3.0 or higher is required");
            }
            if (graphics.msaa_samples < 0 || graphics.msaa_samples > 16)
            {
                throw std::invalid_argument("MSAA samples must be between 0 and 16");
            }
            if (opacity < 0.0f || opacity > 1.0f)
            {
                throw std::invalid_argument("Opacity must be between 0.0 and 1.0");
            }
        }
    };

} // namespace c2l::core

#endif // WINDOW_SETTINGS_HPP