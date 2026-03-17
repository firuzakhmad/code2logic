#include "algorithms/core/json_algorithm_base.hpp"
#include "core/json_config_manager/json_config_manager.hpp"
#include "algorithms/managers/algorithm_manager.hpp"

namespace c2l::algorithms
{
	JsonAlgorithmBase::JsonAlgorithmBase(
		core::JsonConfigManager& json_config_manager, 
        AlgorithmType type)
	{
        m_is_valid = load_metadata_from_json(json_config_manager, type);

        if (!m_is_valid)
        {
            LOG_ERROR(
                "Failed to load metadata for algorithm: {}", 
                algorithm_display_name(type)
            );
        }
	}

	bool JsonAlgorithmBase::load_metadata_from_json(
        core::JsonConfigManager& json_config_manager,
        AlgorithmType expected_type)
	{
        std::unique_lock lock(m_metadata_mutex);

        const auto algorithm_json = fetch_algorithm_json(
            json_config_manager, 
            expected_type
        );
        if (algorithm_json.is_null()) 
        {
            LOG_ERROR(
                "Failed to fetch JSON for algorithm type: {}", 
                algorithm_display_name(expected_type)
            );
            return false;
        }

        bool success = true;
        success &= parse_identity(algorithm_json);
        success &= parse_complexity(algorithm_json);
        success &= parse_properties(algorithm_json);
        success &= parse_description(algorithm_json);
        success &= parse_variables(algorithm_json);
        success &= parse_step_types(algorithm_json);
        success &= parse_step_mappings(algorithm_json);
        success &= parse_visualization(algorithm_json);
        success &= parse_version_info(algorithm_json);

        if (!success)
        {
            LOG_ERROR(
                "Failed to parse one or more metadata sections for: {}", 
                algorithm_display_name(expected_type)
            );
        }

        return success;
	}

    nlohmann::json JsonAlgorithmBase::fetch_algorithm_json(
        core::JsonConfigManager& manager,
        AlgorithmType type
    ) const
    {
	    // Loading algorithm
	    manager.load_algorithm_config(type).get();

	    // Fetching loaded algorithm information from its json file.
        auto algorithm_config = manager.get_algorithm_config(type);

        if (algorithm_config.empty())
        {
            LOG_ERROR("Empty algorithm config for type: {}", static_cast<int>(type));
            return {};
        }

        if (!algorithm_config.contains("algorithm"))
        {
            LOG_ERROR("Algorithm config missing 'algorithm' root object for: {}", 
                     algorithm_id(type));
            return {};
        }

        return algorithm_config["algorithm"];
    }

    bool JsonAlgorithmBase::parse_identity(const nlohmann::json& j)
    {
        if (!j.contains("id"))
        {
            LOG_ERROR("Algorithm JSON missing 'id' field");
            return false;
        }

        const std::string id = j["id"];
        AlgorithmType parsed_type = id_to_algorithm_type(id);

        if (parsed_type == AlgorithmType::UNKNOWN)
        {
            LOG_ERROR("Unknown algorithm id: '{}'", id);
            return false;
        }

        const AlgorithmInfo* info = get_algorithm_info(parsed_type);
        if (!info)
        {
            LOG_ERROR("Algorithm info not found for id: '{}'", id);
            return false;
        }

        m_metadata.id = info->id;
        m_metadata.display_name = info->display_name;
        m_metadata.category = info->category;
        m_metadata.display_category = info->display_category;
        m_metadata.type = info->type;
        m_metadata.display_type = j.value("display_type", std::string(info->display_name));

        return true;
    }

    bool JsonAlgorithmBase::parse_complexity(const nlohmann::json& j)
    {
        if (!j.contains("complexity"))
        {
            LOG_WARNING(
                "Algorithm '{}' missing complexity information", 
                m_metadata.id
            );
            return true; 
        }

        const auto& c = j["complexity"];

        m_metadata.complexity.time_best    = c.value("time_best", "");
        m_metadata.complexity.time_average = c.value("time_average", "");
        m_metadata.complexity.time_worst   = c.value("time_worst", "");
        m_metadata.complexity.space        = c.value("space", "");
        m_metadata.complexity.explanation  = c.value("explanation", "");

        return true;
    }

    bool JsonAlgorithmBase::parse_properties(const nlohmann::json& j)
    {
        if (!j.contains("properties"))
        {
            return true;
        }

        const auto& p = j["properties"];

        m_metadata.properties.stable = p.value("stable", false);
        m_metadata.properties.in_place = p.value("in_place", false);
        m_metadata.properties.adaptive = p.value("adaptive", false);
        m_metadata.properties.comparison_based = p.value("comparison_based", false);
        m_metadata.properties.online = p.value("online", false);
        m_metadata.properties.recursive = p.value("recursive", false);
        m_metadata.properties.parallelizable = p.value("parallelizable", false);
        m_metadata.properties.deterministic = p.value("deterministic", true);

        return true;
    }

    bool JsonAlgorithmBase::parse_description(const nlohmann::json& j)
    {
        if (!j.contains("description")) 
        {
            LOG_WARNING("Algorithm '{}' missing description", m_metadata.id);
            return true;
        }

        const auto& d = j["description"];

        m_metadata.description.brief = d.value("brief", "N/A");

        if (d.contains("detailed"))
            m_metadata.description.detailed = d["detailed"].get<std::vector<std::string>>();

        if (d.contains("pseudocode"))
            m_metadata.description.pseudocode = d["pseudocode"].get<std::vector<std::string>>();

        if (d.contains("optimizations"))
            m_metadata.description.optimizations = d["optimizations"].get<std::vector<std::string>>();

        if (d.contains("use_cases"))
            m_metadata.description.use_cases = d["use_cases"].get<std::vector<std::string>>();

        if (d.contains("disadvantages"))
            m_metadata.description.disadvantages = d["disadvantages"].get<std::vector<std::string>>();
        
        if (d.contains("related_algorithms"))
            m_metadata.description.related_algorithms = d["related_algorithms"].get<std::vector<std::string>>();
        
        return true;
    }

    bool JsonAlgorithmBase::parse_variables(const nlohmann::json& j)
    {
        if (!j.contains("variables")) return true;

        const auto& vars = j["variables"];
        m_metadata.variables.clear();
        m_metadata.variables.reserve(vars.size());

        for (const auto& v : vars)
        {
            AlgorithmVariableInfo var;

            var.name         = v.value("name", "");
            if (var.name.empty())
            {
                LOG_WARNING("Skipping variable with empty name");
                continue;
            }

            var.display_name = v.value("display_name", var.name);
            var.type         = v.value("type", "Unknown");
            var.description  = v.value("description", "");

            if (v.contains("visualization"))
            {
                const auto& viz = v["visualization"];
                var.color = viz.value("color", "#FFFFFF");
                var.icon = viz.value("icon", "");
            }

            if (v.contains("default_value"))
                var.default_value = v["default_value"];

            m_metadata.variables.push_back(std::move(var));
        }

        LOG_DEBUG("Parsed {} variables for algorithm '{}'", 
                 m_metadata.variables.size(), m_metadata.id);

        return true;
    }

    bool JsonAlgorithmBase::parse_step_types(const nlohmann::json& j)
    {
        if (!j.contains("step_types")) 
        {
            return true;
        }

        const auto& steps = j["step_types"];
        m_metadata.step_types.clear();
        m_metadata.step_types.reserve(steps.size());

        for (const auto& s : steps)
        {
            StepTypeInfo step;

            step.id = s.value("id", "");
            if (step.id.empty()) 
            {
                LOG_WARNING("Skipping step type with empty id");
                continue;
            }
            
            step.name = s.value("name", step.id);
            step.description = s.value("description", "");

            if (s.contains("tags"))
                step.tags = s["tags"].get<std::vector<std::string>>();

            if (s.contains("icon"))
                step.icon = s.value("icon", "");

            m_metadata.step_types.push_back(std::move(step));
        }

        LOG_DEBUG(
            "Parsed {} step types for algorithm '{}'", 
            m_metadata.step_types.size(), 
            m_metadata.id
        );

        return true;
    }

    bool JsonAlgorithmBase::parse_step_mappings(const nlohmann::json& j)
    {
        if (!j.contains("step_mappings")) 
        {
            LOG_ERROR(
                "Algorithm '{}' missing step_mappings", 
                m_metadata.id
            );
            return false;
        }

        const auto& mappings = j["step_mappings"];
        m_metadata.step_mappings.clear();
        
        for (auto it = mappings.begin(); it != mappings.end(); ++it)
        {
            StepMappingInfo sm;
            const auto& m = it.value();

            sm.pseudocode_line      = m.value("pseudocode_line", static_cast<size_t>(0));
            sm.code_line            = m.value("code_line", static_cast<size_t>(0));
            sm.code_line_content    = m.value("code_line_content", "");
            sm.description_template = m.value("description_template", "");

            if (m.contains("extract_variables"))
                sm.extract_variables = m["extract_variables"].get<std::vector<std::string>>();

            if (m.contains("tags"))
                sm.tags = m["tags"].get<std::vector<std::string>>();
        
            if (m.contains("condition"))
                sm.condition = m["condition"];

            if (sm.pseudocode_line == 0 || sm.code_line_content.empty())
            {
                LOG_WARNING(
                    "Incomplete step mapping for key '{}'", 
                    it.key()
                );
                continue;
            }

            m_metadata.step_mappings.emplace(it.key(), std::move(sm));
        }

        LOG_DEBUG(
            "Parsed {} step mappings for algorithm '{}'", 
            m_metadata.step_mappings.size(), 
            m_metadata.id
        );

        return !m_metadata.step_mappings.empty();
    }

    bool JsonAlgorithmBase::parse_visualization(const nlohmann::json& j)
    {
        if (!j.contains("visualization"))
        {
            return true;
        }

        const auto& v = j["visualization"];

        m_metadata.visualization.default_style = v.value("default_style", "bars");
        
        if (v.contains("highlight_colors"))
        {
            const auto& colors = v["highlight_colors"];
            m_metadata.visualization.highlight_colors.current = colors.value("current", "#FF6B6B");
            m_metadata.visualization.highlight_colors.compared = colors.value("compared", "#4ECDC4");
            m_metadata.visualization.highlight_colors.swapped = colors.value("swapped", "#FFD166");
            m_metadata.visualization.highlight_colors.sorted = colors.value("sorted", "#06D6A0");
            m_metadata.visualization.highlight_colors.pivot = colors.value("pivot", "#9B59B6");
            m_metadata.visualization.highlight_colors.visited = colors.value("visited", "#3498DB");
            m_metadata.visualization.highlight_colors.frontier = colors.value("frontier", "#E67E22");
        }

        m_metadata.visualization.animation_speed = v.value("animation_speed", 1.0);
        m_metadata.visualization.show_labels = v.value("show_labels", true);
        m_metadata.visualization.show_values = v.value("show_values", true);
        m_metadata.visualization.show_indices = v.value("show_indices", false);

        return true;
    }

    bool JsonAlgorithmBase::parse_version_info(const nlohmann::json& j)
    {
        if (j.contains("version"))
            m_metadata.version = j["version"];
        
        if (j.contains("last_updated"))
            m_metadata.last_updated = j["last_updated"];
        
        return true;
    }


    // Algorithm metadata
	    const std::string& JsonAlgorithmBase::get_id() const noexcept 
    { 
        std::shared_lock lock(m_metadata_mutex);
        return m_metadata.id; 
    }

    const std::string& JsonAlgorithmBase::get_display_name() const noexcept
    { 
        std::shared_lock lock(m_metadata_mutex);
        return m_metadata.display_name; 
    }

    AlgorithmCategory JsonAlgorithmBase::get_category() const noexcept
    { 
        std::shared_lock lock(m_metadata_mutex);
        return m_metadata.category; 
    }

    const std::string& JsonAlgorithmBase::get_display_category() const noexcept
    { 
        std::shared_lock lock(m_metadata_mutex);
        return m_metadata.display_category; 
    }

    AlgorithmType JsonAlgorithmBase::get_type() const noexcept 
    { 
        std::shared_lock lock(m_metadata_mutex);
        return m_metadata.type; 
    }

    const std::string& 
    JsonAlgorithmBase::get_display_type() const noexcept
    { 
        std::shared_lock lock(m_metadata_mutex);
        return m_metadata.display_type; 
    }

    const AlgorithmComplexityInfo& 
    JsonAlgorithmBase::get_complexity() const noexcept 
    { 
        std::shared_lock lock(m_metadata_mutex);
        return m_metadata.complexity; 
    }

    const AlgorithmPropertiesInfo& 
    JsonAlgorithmBase::get_properties() const noexcept
    { 
        std::shared_lock lock(m_metadata_mutex);
        return m_metadata.properties; 
    }

    const AlgorithmDescriptionInfo& 
    JsonAlgorithmBase::get_description() const noexcept
    { 
        std::shared_lock lock(m_metadata_mutex);
        return m_metadata.description; 
    }

    const std::vector<AlgorithmVariableInfo>& 
    JsonAlgorithmBase::get_variable_info() const noexcept
    { 
        std::shared_lock lock(m_metadata_mutex);
        return m_metadata.variables; 
    }

    const std::vector<StepTypeInfo>& 
    JsonAlgorithmBase::get_step_types() const noexcept
    { 
        std::shared_lock lock(m_metadata_mutex);
        return m_metadata.step_types; 
    }

    const VisualizationConfig& 
    JsonAlgorithmBase::get_visualization_config() const noexcept
    { 
        std::shared_lock lock(m_metadata_mutex);
        return m_metadata.visualization; 
    }

    bool JsonAlgorithmBase::has_step_mapping(const std::string& step_id) const
    {
        std::shared_lock lock(m_metadata_mutex);
        return m_metadata.has_step_mapping(step_id);
    }

    std::optional<StepMappingInfo> JsonAlgorithmBase::get_step_mapping(
        const std::string& step_id
    ) const
    {
        std::shared_lock lock(m_metadata_mutex);

        if (const auto* mapping = m_metadata.get_step_mapping(step_id))
        {
            return *mapping;
        }

        return std::nullopt;
    }

    std::optional<AlgorithmVariableInfo> 
    JsonAlgorithmBase::get_variable_info(const std::string& name) const
    {
        std::shared_lock lock(m_metadata_mutex);
        return m_metadata.get_variable_info(name);
    }

    bool JsonAlgorithmBase::is_valid() const noexcept
    {
        std::shared_lock lock(m_metadata_mutex);
        return m_is_valid && m_metadata.is_valid();
    }

    std::string JsonAlgorithmBase::format_step_description(
        const std::string& step_id,
        const AlgorithmStep& step
    ) const
    {
        auto mapping_opt = get_step_mapping(step_id);
        if (!mapping_opt)
        {
            return step.description;
        }

        const auto& mapping = *mapping_opt;
        auto variables = extract_variables(step_id, step);

        return mapping.format_description(variables);
    }

    std::vector<CodeHighlight> JsonAlgorithmBase::generate_highlights(
        const std::string& step_id,
        const AlgorithmStep& step
    ) const 
    {
        std::vector<CodeHighlight> highlights;

        auto mapping_opt = get_step_mapping(step_id);
        if (!mapping_opt)
        {
            return highlights;
        }

        const auto& mapping = *mapping_opt;
        auto variables = extract_variables(step_id, step);

        // Splitting variables into indices and values
        std::unordered_map<std::string, std::string> indices;
        std::unordered_map<std::string, std::string> values;

        for (const auto& [key, value] : variables)
        {
            if (key.find("arr[") != std::string::npos ||
                key.find("]") != std::string::npos ||
                key.find("value") != std::string::npos)
            {
                values[key] = value;
            }
            else 
            {
                indices[key] = value;
            }
        }

        CodeHighlight highlight(
            mapping.pseudocode_line,
            mapping.code_line_content,
            format_step_description(step_id, step),
            indices,
            values,
            true
        );

        highlights.push_back(std::move(highlight));
        return highlights;  
    }

    std::unordered_map<std::string, std::string> 
    JsonAlgorithmBase::extract_variables(
        const std::string& step_id,
        const AlgorithmStep& step
    ) const
    {
        std::unordered_map<std::string, std::string> result;
        
        auto mapping_opt = get_step_mapping(step_id);
        if (!mapping_opt)
            return result;

        const auto& mapping = *mapping_opt;

        for (const auto& var_name : mapping.extract_variables)
        {
            if (auto int_val = step.metadata.get<int>(var_name))
            {
                result[var_name] = std::to_string(*int_val);
            }
            else if (auto size_t_val = step.metadata.get<size_t>(var_name))
            {
                result[var_name] = std::to_string(*size_t_val);
            }
            else if (auto boo_val = step.metadata.get<bool>(var_name))
            {
                result[var_name] = *boo_val ? "true" : "false";
            }
            else if (auto string_val = step.metadata.get<std::string>(var_name))
            {
                result[var_name] = *string_val;
            }
            else
            {
                LOG_DEBUG("Could not find variable: {}", var_name);
            }
        }

        return result;
    }

    std::optional<size_t> JsonAlgorithmBase::get_pseudocode_line(
        const std::string& step_id
    ) const 
    {
        auto mapping_opt = get_step_mapping(step_id);
        if (mapping_opt)
        {
            return mapping_opt->pseudocode_line;
        }

        return std::nullopt;
    }

    std::vector<std::string> JsonAlgorithmBase::get_step_tags(
        const std::string& step_id
    ) const
    {
        auto mapping_opt = get_step_mapping(step_id);
        if (mapping_opt)
        {
            return mapping_opt->tags;
        }

        return {};
    }

    const IAlgorithmMetadata*
    JsonAlgorithmBase::metadata() const noexcept
	{
	    return this;
	}

} // namespace c2l::algorithms