#ifndef CODE2LOGIC_JSON_ALGORITHM_BASE_HPP
#define CODE2LOGIC_JSON_ALGORITHM_BASE_HPP

#include "algorithms/algorithm_base.hpp"
#include "algorithms/i_algorithm_metadata.hpp"
#include "algorithms/algorithm_metadata_types.hpp"
#include "algorithms/code_highlight.hpp"

#include <string>
#include <vector>
#include <shared_mutex>

namespace c2l::algorithms
{
    /**
     * @brief Base class for all JSON-configured algorithms
     * 
     * This class provides complete metadata loading and management from
     * JSON configuration files. It implements IAlgorithmMetadata and
     * serves as the foundation for all algorithm implementations.
     */
	class JsonAlgorithmBase : public AlgorithmBase, public IAlgorithmMetadata
	{
	public:
		explicit JsonAlgorithmBase(
            core::JsonConfigManager& json_config_manager,
            AlgorithmType type
        );
		~JsonAlgorithmBase() override = default;

        JsonAlgorithmBase(const JsonAlgorithmBase&) = delete;
        JsonAlgorithmBase& operator=(const JsonAlgorithmBase&) = delete;
        JsonAlgorithmBase(JsonAlgorithmBase&&) noexcept = delete;
        JsonAlgorithmBase& operator=(JsonAlgorithmBase&&) noexcept = delete;

        // IAlgorithmMetadata implementation
		[[nodiscard]] const std::string& get_id() const noexcept override;
        [[nodiscard]] const std::string& get_display_name() const noexcept override;
        [[nodiscard]] AlgorithmCategory get_category() const noexcept override;
        [[nodiscard]] const std::string& get_display_category() const noexcept override;
        [[nodiscard]] AlgorithmType get_type() const noexcept override;
        [[nodiscard]] const std::string& get_display_type() const noexcept override;
        
        [[nodiscard]] const AlgorithmComplexityInfo& 
        get_complexity() const noexcept override;
        [[nodiscard]] const AlgorithmPropertiesInfo& 
        get_properties() const noexcept override;
        [[nodiscard]] const AlgorithmDescriptionInfo& 
        get_description() const noexcept override;
        
        [[nodiscard]] const std::vector<AlgorithmVariableInfo>& 
        get_variable_info() const noexcept override;
        [[nodiscard]] const std::vector<StepTypeInfo>&  
        get_step_types() const noexcept override;
        [[nodiscard]] const VisualizationConfig& 
        get_visualization_config() const noexcept override;

        [[nodiscard]] bool has_step_mapping(
            const std::string& step_id
        ) const override;
        [[nodiscard]] std::optional<StepMappingInfo> 
        get_step_mapping(const std::string& step_id) const override;
        [[nodiscard]] std::optional<AlgorithmVariableInfo> 
        get_variable_info(const std::string& name) const override;
        
        [[nodiscard]] bool is_valid() const noexcept override;

        // Helper methods
        /**
         * @brief Format a description using the step mapping and current variables 
         */
        [[nodiscard]] std::string format_step_description(
            const std::string& step_id,
            const AlgorithmStep& step
        ) const;

        /**
         * @brief Generate code highlights for a specific step
         */
        [[nodiscard]] std::vector<CodeHighlight> generate_highlights(
            const std::string& step_id,
            const AlgorithmStep& step
        ) const;

        /**
         * @brief Extract variables from step based on step mapping
         */
        [[nodiscard]] std::unordered_map<std::string, std::string> extract_variables(
            const std::string& step_id,
            const AlgorithmStep& step
        ) const;

        /**
         * @brief Get the pseudocode line for a specific step
         */
        [[nodiscard]] std::optional<size_t> get_pseudocode_line(
            const std::string& step_id
        ) const;

        /**
         * @brief Get all tags for a step
         */
        [[nodiscard]] std::vector<std::string> get_step_tags(
            const std::string& step_id
        ) const;


    protected:
        /**
         * @brief Load metadata from JSON configuration
         * @return true if loading succeeded
         */
        bool load_metadata_from_json(
            core::JsonConfigManager& json_config_manager,
            AlgorithmType expected_type
        );

        /**
         * @brief Get the raw JSON for this algorithm
         */
        nlohmann::json fetch_algorithm_json(
            core::JsonConfigManager& manager,
            AlgorithmType type
        ) const;

        bool parse_identity(const nlohmann::json& j);
        bool parse_complexity(const nlohmann::json& j);
        bool parse_properties(const nlohmann::json& j);
        bool parse_description(const nlohmann::json& j);
        bool parse_variables(const nlohmann::json& j);
        bool parse_step_types(const nlohmann::json& j);
        bool parse_step_mappings(const nlohmann::json& j);
        bool parse_visualization(const nlohmann::json& j);
        bool parse_version_info(const nlohmann::json& j);

    private:
        mutable std::shared_mutex m_metadata_mutex;

        AlgorithmMetadata m_metadata    {};

        bool m_is_valid                 {false};
	};

} // namespace c2l::algorithms

#endif // CODE2LOGIC_JSON_ALGORITHM_BASE_HPP