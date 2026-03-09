#ifndef CODE2LOGIC_ALGORITHM_METADATA_TYPES_HPP
#define CODE2LOGIC_ALGORITHM_METADATA_TYPES_HPP

#include "algorithms/algorithm_types.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <stddef.h>

#include <imgui.h>

namespace c2l::algorithms
{
    struct AlgorithmComplexityInfo
    {
        std::string time_best;
        std::string time_average;
        std::string time_worst;
        std::string space;
        std::string explanation;

        [[nodiscard]] bool is_valid() const noexcept
        {
            return !time_best.empty() &&
                   !time_worst.empty() &&
                   !space.empty();
        }
    };

    struct AlgorithmPropertiesInfo
    {
        bool stable             {false};
        bool in_place           {false};
        bool adaptive           {false};
        bool comparison_based   {false};
        bool online             {false};
        bool recursive          {false};
        bool parallelizable     {false};
        bool deterministic      {false};

        [[nodiscard]] std::vector<std::pair<std::string, bool>> 
        get_as_key_value() const noexcept;
    };

    struct AlgorithmVariableInfo
	{
		std::string name;
        std::string display_name;
        std::string type;
        std::string description;
        std::string color;
        std::string icon;
        std::optional<std::string> default_value;

        [[nodiscard]] bool is_valid() const noexcept 
        { 
            return !name.empty();
        }
	};

    struct StepTypeInfo
	{
		std::string id;
        std::string name;
        std::string description;
        std::vector<std::string> tags;
        std::optional<std::string> icon;

        [[nodiscard]] bool is_valid() const noexcept
        {
            return !id.empty();
        }

        [[nodiscard]] bool has_tag(const std::string& tag) const;
	};

    struct StepMappingInfo
	{
		size_t pseudocode_line  {0};
		size_t code_line        {0};
        std::string code_line_content;
        std::string description_template;
        std::vector<std::string> extract_variables;
        std::vector<std::string> tags;
        std::optional<std::string> condition;

        [[nodiscard]] bool is_valid() const noexcept
        {
            return pseudocode_line > 0 && !code_line_content.empty();
        }

        [[nodiscard]] std::string format_description(
            const std::unordered_map<std::string, std::string>& variables
        ) const;
	};

    struct AlgorithmDescriptionInfo
    {
        std::string brief;
        std::vector<std::string> detailed;
        std::vector<std::string> pseudocode;
        std::vector<std::string> optimizations;
        std::vector<std::string> use_cases;
        std::vector<std::string> disadvantages;
        std::vector<std::string> related_algorithms;

        [[nodiscard]] bool has_pseudocode() const noexcept
        {
            return !pseudocode.empty();
        }

        [[nodiscard]] size_t pseudocode_line_count() const noexcept
        {
            return pseudocode.size();
        }
    };

    struct VisualizationConfig
    {
        std::string default_style   {"bars"};
        
        struct HighlightColors
        {
            std::string current     {"#FF6B6B"};
            std::string compared    {"#4ECDC4"};
            std::string swapped     {"#FFD166"};
            std::string sorted      {"#06D6A0"};
            std::string pivot       {"#9B59B6"};
            std::string visited     {"#3498DB"};
            std::string frontier    {"#E67E22"};
        } highlight_colors;
        
        double animation_speed      {1.0};
        bool show_labels            {true};
        bool show_values            {true};
        bool show_indices           {false};
        
        [[nodiscard]] ImU32 get_color(const std::string& key) const;
    };

    struct AlgorithmMetadata
    {
        std::string id;
        std::string display_name;
        AlgorithmCategory category      { AlgorithmCategory::UNKNOWN };
        std::string display_category;
        AlgorithmType type              { AlgorithmType::UNKNOWN };
        std::string display_type;
 
        AlgorithmComplexityInfo complexity;
        AlgorithmPropertiesInfo properties;
        AlgorithmDescriptionInfo description;

        std::vector<AlgorithmVariableInfo> variables;
        std::vector<StepTypeInfo> step_types;
        std::unordered_map<std::string, StepMappingInfo> step_mappings;
        VisualizationConfig visualization;

        std::string version;
        std::string last_updated;

        [[nodiscard]] bool is_valid() const noexcept 
        {
            return !id.empty();
        }

        [[nodiscard]] bool has_step_mapping(const std::string& step_id) const;

        [[nodiscard]] const StepMappingInfo* 
        get_step_mapping(const std::string& step_id) const;

        [[nodiscard]] std::optional<AlgorithmVariableInfo> 
        get_variable_info(const std::string& name) const;

    };

} // // namespace c2l::algorithms

#endif // CODE2LOGIC_ALGORITHM_METADATA_TYPES_HPP