#ifndef CODE2LOGIC_CODE_HIGHLIGHT_HPP
#define CODE2LOGIC_CODE_HIGHLIGHT_HPP

#include <string>
#include <glm/glm.hpp>
#include <unordered_map>
#include <utility>

namespace c2l::algorithms
{
    struct CodeHighlight
    {
        size_t line_number;
        std::string code_line;
        std::string description;
        bool is_active;
        glm::vec4 highlight_color;
        std::unordered_map<std::string, std::string> index_variables;
        std::unordered_map<std::string, std::string> variable_values;

        CodeHighlight(size_t line,
            std::string code,
            std::string desc = "",
            bool active = false,
            glm::vec4 color = {1.0f, 0.8f, 0.0f, 0.3f})
            : line_number(line)
            , code_line(std::move(code))
            , description(std::move(desc))
            , is_active(active)
            , highlight_color(color)
        {}

    // New constructor with variable values
        CodeHighlight(size_t line,
            std::string code,
            std::string desc,
            std::unordered_map<std::string, std::string> index_variables,
            std::unordered_map<std::string, std::string> variable_values,
            bool active = false,
            glm::vec4 color = {1.0f, 0.8f, 0.0f, 0.3f})
            : line_number(line)
            , code_line(std::move(code))
            , description(std::move(desc))
            , is_active(active)
            , highlight_color(color)
            , index_variables(std::move(index_variables))
            , variable_values(std::move(variable_values))
        {}
    };

    struct PseudocodeDisplay
    {
        std::vector<std::string> lines;
        std::vector<size_t> highlighted_lines;
        std::unordered_map<size_t, std::unordered_map<std::string, std::string>> line_index_values;
        std::unordered_map<size_t, std::unordered_map<std::string, std::string>> line_variable_values;
    };
} // namespace c2l::algorithms

#endif // CODE2LOGIC_CODE_HIGHLIGHT_HPP