#ifndef CORE_UTILS_UTILS_HPP
#define CORE_UTILS_UTILS_HPP

#include "core/utils/variables.hpp"

#include <glad/glad.h>
#include <imgui.h>
#include <cstdint>

namespace c2l::core::utils
{
    /**
     * @brief Convert OpenGL texture ID to ImGui texture ID
     */
    inline ImTextureID gl_texture_to_im_texture(GLuint texture_id)
    {
        return static_cast<ImTextureID>(texture_id);
    }

    /**
     * @brief Convert ImGui texture ID back to OpenGL texture ID
     */
    inline GLuint im_texture_to_gl_texture(ImTextureID texture_id)
    {
        return static_cast<GLuint>(texture_id);
    }

    inline void heading_text(
        const char* text,
        const float scale = DEFAULT_HEADING_FONT_SCALE,
        const ImVec2& margin = DEFAULT_HEADING_FONT_MARGIN)
    {
        if (!ImGui::GetCurrentContext())
            return;

        if (scale != 1.0f)
            ImGui::SetWindowFontScale(scale);

        ImVec2 cursor = ImGui::GetCursorPos();
        cursor.x += margin.x;
        cursor.y += margin.y;
        ImGui::SetCursorPos(cursor);

        ImGui::TextUnformatted(text);

        if (scale != 1.0f)
            ImGui::SetWindowFontScale(1.0f);
    }

    inline void heading_colored_text(
        const char* text,
        const ImVec4& color = DEFAULT_HEADING_FONT_COLOR,
        const float scale = DEFAULT_HEADING_FONT_SCALE,
        const ImVec2& margin = DEFAULT_HEADING_FONT_MARGIN)
    {
        if (!ImGui::GetCurrentContext())
            return;

        if (scale != 1.0f)
            ImGui::SetWindowFontScale(scale);

        ImVec2 cursor = ImGui::GetCursorPos();
        cursor.x += margin.x;
        cursor.y += margin.y;
        ImGui::SetCursorPos(cursor);

        ImGui::TextColored(color, "%s", text);

        if (scale != 1.0f)
            ImGui::SetWindowFontScale(1.0f);
    }

    inline ImVec4 hex_to_vec4(const std::string& hex)
    {
        if (hex.size() != 7 || hex[0] != '#')
            return ImVec4(1, 1, 1, 1); // fallback white

        int r = std::stoi(hex.substr(1, 2), nullptr, 16);
        int g = std::stoi(hex.substr(3, 2), nullptr, 16);
        int b = std::stoi(hex.substr(5, 2), nullptr, 16);

        return ImVec4(
            r / 255.0f,
            g / 255.0f,
            b / 255.0f,
            1.0f
        );
    }

    inline ImU32 hex_to_u32(const std::string& hex)
    {
        ImVec4 v = hex_to_vec4(hex);
        return ImGui::ColorConvertFloat4ToU32(v);
    }

} // namespace c2l::core::utils

#endif // CORE_UTILS_UTILS_HPP
