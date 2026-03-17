//
// Created by Akhmad on 3/14/26.
//

#include "algorithm_registry.hpp"
#include "algorithms/bubble_sort.hpp"
#include "algorithms/quick_sort.hpp"

#include <algorithm>

#include "algorithms/visualizers/array_based_visualizer.hpp"


namespace c2l::algorithms
{
    AlgorithmRegistry::AlgorithmRegistry(
        core::JsonConfigManager &json_config_manager)
            : m_json_config_manager{json_config_manager}
    {
        m_is_valid = create_entry_for_available_algorithms();

        if (!m_is_valid)
        {
            LOG_ERROR(
                "Failed to load or create available algorithm using JSON file."
            );
        }
    }

    void AlgorithmRegistry::register_algorithm(AlgorithmEntry entry)
    {
        std::unique_lock lock(m_mutex);

        m_entries.emplace(entry.type, std::move(entry));

        // Checking available algorithm types list
        if (std::find(m_available_algorithm_types.begin(),
            m_available_algorithm_types.end(),
            entry.type) == m_available_algorithm_types.end())
        {
            m_available_algorithm_types.push_back(entry.type);
        }

        // Checking available algorithm names list
        if (std::find(m_available_algorithms_names.begin(),
            m_available_algorithms_names.end(),
            entry.display_name) == m_available_algorithms_names.end())
        {
            m_available_algorithms_names.push_back(entry.display_name);
        }

        if (const auto* info = get_algorithm_info(entry.type))
        {
            auto& vec =
                m_available_categorized_algorithms[std::string(info->display_category)];
            if (std::find(vec.begin(), vec.end(), info) == vec.end())
            {
                vec.push_back(info);
            }
        }
    }

    void AlgorithmRegistry::unregister_algorithm(AlgorithmEntry entry)
    {
        std::unique_lock lock(m_mutex);

        m_entries.erase(entry.type);

        // Removing from types list
        auto type_it = std::find(
            m_available_algorithm_types.begin(),
            m_available_algorithm_types.end(),
            entry.type
        );
        if (type_it != m_available_algorithm_types.end())
        {
            m_available_algorithm_types.erase(type_it);
        }

        // Removing from legacy names
        std::string name = std::string(algorithm_display_name(entry.type));
        auto name_it = std::find(
            m_available_algorithms_names.begin(),
            m_available_algorithms_names.end(),
            name)
        ;
        if (name_it != m_available_algorithms_names.end())
        {
            m_available_algorithms_names.erase(name_it);
        }


        if (const auto* info = get_algorithm_info(entry.type))
        {
            auto it = m_available_categorized_algorithms.find(std::string(info->display_category));
            if (it != m_available_categorized_algorithms.end())
            {
                auto& vec = it->second;

                vec.erase(
                    std::remove(vec.begin(), vec.end(), info),
                    vec.end()
                );

                if (vec.empty())
                    m_available_categorized_algorithms.erase(it);
            }
        }
    }

    std::unique_ptr<ISimpleAlgorithm>
    AlgorithmRegistry::create_algorithm(
    const AlgorithmType& type) const
    {
        std::shared_lock lock(m_mutex);

        auto it = m_entries.find(type);

        if (it == m_entries.end() || !it->second.algorithm_factory)
            return nullptr;

        return it->second.algorithm_factory(m_json_config_manager);
    }

    std::unique_ptr<IAlgorithmVisualizer>
    AlgorithmRegistry::create_visualizer(
        const AlgorithmType& type,
        const VisualizationConfig& visualization_config) const
    {
        std::shared_lock lock(m_mutex);

        auto it = m_entries.find(type);

        if (it == m_entries.end() || !it->second.visualizer_factory)
            return nullptr;

        return it->second.visualizer_factory(visualization_config);
    }

    nlohmann::json AlgorithmRegistry::fetch_available_algorithms_json() const
    {
        m_json_config_manager.load_available_algorithms_config().get();

        // Fetching loaded available algorithm information from its json file.
        auto available_algorithm_config =
            m_json_config_manager.get_available_algorithms_config();

        if (available_algorithm_config.empty())
        {
            LOG_ERROR("Empty Available algorithm config");
            return {};
        }

        if (!available_algorithm_config.contains("available_algorithms"))
        {
            LOG_ERROR(
                "Available algorithm config missing 'available_algorithms' root object."
            );
            return {};
        }

        return available_algorithm_config["available_algorithms"];
    }

    bool AlgorithmRegistry::create_entry_for_available_algorithms()
    {
        const auto available_algorithms = fetch_available_algorithms_json();

        if (!available_algorithms.is_array())
        {
            LOG_ERROR("Invalid available algorithms JSON structure.");
            return false;
        }

        for (const auto& v : available_algorithms)
        {
            AlgorithmEntry entry{};

            const std::string id = v.value("id", "");
            if (id.empty())
            {
                LOG_WARNING("Skipping algorithm with empty id");
                continue;
            }

            const AlgorithmType parsed_type = id_to_algorithm_type(id);
            if (parsed_type == AlgorithmType::UNKNOWN)
            {
                LOG_ERROR("Unknown algorithm id: '{}'", id);
                continue;
            }

            const AlgorithmInfo* info = get_algorithm_info(parsed_type);
            if (!info)
            {
                LOG_ERROR("Algorithm info not found for id: '{}'", id);
                continue;
            }

            entry.type = info->type;
            entry.id = info->id;
            entry.display_name = info->display_name;
            entry.category = info->category;
            entry.display_category = info->display_category;

            // Algorithm factory
            entry.algorithm_factory =
                [this, parsed_type](core::JsonConfigManager&)
                -> std::unique_ptr<ISimpleAlgorithm>
                {
                    switch (parsed_type)
                    {
                        case AlgorithmType::BUBBLE_SORT:
                            return std::make_unique<BubbleSort>(m_json_config_manager);

                        case AlgorithmType::QUICK_SORT:
                            return std::make_unique<QuickSort>(m_json_config_manager);

                        default:
                            return nullptr;
                    }
                };

            entry.visualizer_factory =
                [parsed_type](const VisualizationConfig& visualization_config)
                -> std::unique_ptr<IAlgorithmVisualizer>
                {
                    switch (auto category = algorithm_category(parsed_type)) {
                        case AlgorithmCategory::SORTING:
                        case AlgorithmCategory::SEARCHING:
                            return std::make_unique<ArrayBasedVisualizer>(visualization_config);

                        case AlgorithmCategory::GRAPH:
                            // return std::make_unique<GraphBasedVisualizer>(config);

                        case AlgorithmCategory::TREE:
                            // return std::make_unique<TreeBasedVisualizer>(config);

                        default:
                            return nullptr;
                    }
                };

            register_algorithm(std::move(entry));
        }

        return true;
    }

    bool AlgorithmRegistry::has_algorithm(
        const AlgorithmType& type
    ) const noexcept
    {
        std::shared_lock lock(m_mutex);

        return m_entries.find(type) != m_entries.end();
    }

    const std::vector<AlgorithmType>
    AlgorithmRegistry::get_available_algorithm_types() const
    {
        std::shared_lock lock(m_mutex);

        return m_available_algorithm_types;
    }

    const std::vector<std::string>
    AlgorithmRegistry::get_available_algorithm_names() const
    {
        std::shared_lock lock(m_mutex);

        return m_available_algorithms_names;
    }

    const AlgorithmRegistry::CategorizedAlgorithms
    AlgorithmRegistry::get_available_categorized_algorithms() const
    {
        std::shared_lock lock(m_mutex);

        return m_available_categorized_algorithms;
    }

} // namespace c2l::algorithms