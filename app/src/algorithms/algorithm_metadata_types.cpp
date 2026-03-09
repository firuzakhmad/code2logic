#include "algorithms/algorithm_metadata_types.hpp"

#include <algorithm>

namespace c2l::algorithms
{
    std::vector<std::pair<std::string, bool>> 
    AlgorithmPropertiesInfo::get_as_key_value() const noexcept
    {
        return {
            {"Stable", stable},
            {"In-Place", in_place},
            {"Adaptive", adaptive},
            {"Comparison-Based", comparison_based},
            {"Online", online},
            {"Recursive", recursive},
            {"Parallelizable", parallelizable},
            {"Deterministic", deterministic}
        };
    }

    bool StepTypeInfo::has_tag(const std::string& tag) const
    {
        return std::find(tags.begin(), tags.end(), tag) != tags.end();
    }

    std::string StepMappingInfo::format_description(
        const std::unordered_map<std::string, std::string>& variables
    ) const
    {
        std::string result = description_template;

        for (const auto&[key, value] : variables)
        {
            std::string placeholder = "{" + key + "}";
            size_t pos = 0;
            while ((pos = result.find(placeholder, pos)) != std::string::npos)
            {
                result.replace(pos, placeholder.length(), value);
                pos += value.length();
            }
        }

        return result;
    }

    ImU32 VisualizationConfig::get_color(const std::string& key) const
    {
        static const std::unordered_map<std::string, std::string> color_map = {
            {"current", highlight_colors.current},
            {"compared", highlight_colors.compared},
            {"swapped", highlight_colors.swapped},
            {"sorted", highlight_colors.sorted},
            {"pivot", highlight_colors.pivot},
            {"visited", highlight_colors.visited},
            {"frontier", highlight_colors.frontier}
        };
        
        auto it = color_map.find(key);
        if (it != color_map.end())
        {
            // Convert hex string to ImU32
            std::string hex = it->second;
            if (hex.length() == 7 && hex[0] == '#')
            {
                unsigned int r, g, b;
                if (sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b) == 3)
                {
                    return IM_COL32(r, g, b, 255);
                }
            }
        }
        
        return IM_COL32(255, 255, 255, 255);
    }

    bool AlgorithmMetadata::has_step_mapping(const std::string& step_id) const
    {
        return step_mappings.find(step_id) != step_mappings.end();
    }

    const StepMappingInfo* AlgorithmMetadata::get_step_mapping(
        const std::string& step_id
    ) const 
    {
        auto it = step_mappings.find(step_id);
        return it != step_mappings.end() ? &it->second : nullptr;
    }

    std::optional<AlgorithmVariableInfo> AlgorithmMetadata::get_variable_info(
        const std::string& name
    ) const
    {
        auto it = std::find_if(variables.begin(), variables.end(),
            [&name](const AlgorithmVariableInfo& var)
            {
                return var.name == name;
            });
        
        return it != variables.end() 
            ? std::optional<AlgorithmVariableInfo>(*it) 
            : std::nullopt;
    }

} // namespace c2l::algorithms