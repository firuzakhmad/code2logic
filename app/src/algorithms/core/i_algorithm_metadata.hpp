#ifndef CODE2LOGIC_I_ALGORITHM_METADATA_HPP
#define CODE2LOGIC_I_ALGORITHM_METADATA_HPP

#include <string>
#include <vector>
#include <optional>

#include "algorithms/core/algorithm_types.hpp"
#include "algorithms/core/algorithm_metadata_types.hpp"

namespace c2l::algorithms
{
    /**
     * @brief Pure interface for algorithm metadata
     * 
     * This interface provides read-only access to all algorithm metadata
     * loaded from JSON configuration files.
     */
    class IAlgorithmMetadata 
    {
    public:
        virtual ~IAlgorithmMetadata() = default;

        [[nodiscard]] virtual const std::string&
        get_id() const noexcept = 0;
        [[nodiscard]] virtual const std::string&
        get_display_name() const noexcept = 0;
        [[nodiscard]] virtual AlgorithmCategory
        get_category() const noexcept = 0;
        [[nodiscard]] virtual const std::string&
        get_display_category() const noexcept = 0;
        [[nodiscard]] virtual AlgorithmType
        get_type() const noexcept = 0;
        [[nodiscard]] virtual const std::string&
        get_display_type() const noexcept = 0;
        [[nodiscard]] virtual VisualizationType
        get_visualization_type() const noexcept = 0;
        [[nodiscard]] virtual const std::string&
        get_display_visualization() const noexcept = 0;

        [[nodiscard]] virtual const AlgorithmComplexityInfo& 
        get_complexity() const noexcept = 0;
        [[nodiscard]] virtual const AlgorithmPropertiesInfo& 
        get_properties() const noexcept = 0;
        [[nodiscard]] virtual const AlgorithmDescriptionInfo& 
        get_description() const noexcept = 0;

        [[nodiscard]] virtual const std::vector<AlgorithmVariableInfo>& 
        get_variable_info() const noexcept = 0;
        [[nodiscard]] virtual std::optional<AlgorithmVariableInfo>
        get_variable_info(const std::string& name) const = 0;
        [[nodiscard]] virtual const std::vector<StepTypeInfo>& 
        get_step_types() const noexcept = 0;
        [[nodiscard]] virtual const VisualizationConfig& 
        get_visualization_config() const noexcept = 0;

        [[nodiscard]] virtual bool has_step_mapping(
            const std::string& step_id
        ) const = 0;
        [[nodiscard]] virtual std::optional<StepMappingInfo>
        get_step_mapping(const std::string& step_id) const = 0;

        [[nodiscard]] virtual bool is_valid() const noexcept = 0;
    };

}  // namespace c2l::algorithms

#endif // CODE2LOGIC_I_ALGORITHM_METADATA_HPP
