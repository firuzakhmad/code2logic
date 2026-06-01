//
// Created by Akhmad on 3/14/26.
//

#ifndef CODE2LOGIC_ALGORITHM_REGISTRY_HPP
#define CODE2LOGIC_ALGORITHM_REGISTRY_HPP

#include "algorithms/core/i_simple_algorithm.hpp"
#include "algorithms/visualizers/i_algorithm_visualizer.hpp"
#include "core/json_config_manager/json_config_manager.hpp"
#include "algorithms/core/algorithm_types.hpp"
#include "algorithms/core/algorithm_metadata_types.hpp"
#include "ui/managers/icon_manager.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <shared_mutex>
#include <functional>
#include <vector>
#include <unordered_map>
#include <string_view>

#include "ui/core/popup.hpp"


namespace c2l::algorithms
{
    class AlgorithmRegistry final
    {
    public:
        using AlgorithmFactory = std::function<
            std::unique_ptr<ISimpleAlgorithm>(core::JsonConfigManager&)
        >;
        using VisualizerFactory = std::function<
            std::unique_ptr<IAlgorithmVisualizer>(const VisualizationConfig&)
        >;
        using CategorizedAlgorithms = std::unordered_map<
            std::string,
            std::vector<const AlgorithmInfo*>
        >;

        struct AlgorithmEntry
        {
            AlgorithmType type;
            std::string id;
            std::string display_name;
            AlgorithmCategory category;
            std::string display_category;
            AlgorithmFactory algorithm_factory;
            VisualizerFactory visualizer_factory;
        };

        AlgorithmRegistry(
            core::JsonConfigManager& json_config_manager,
            ui::managers::IconManager& icon_manager);
        ~AlgorithmRegistry() = default;

        AlgorithmRegistry(const AlgorithmRegistry&) = delete;
        AlgorithmRegistry& operator=(const AlgorithmRegistry&) = delete;
        AlgorithmRegistry(AlgorithmRegistry&&) = delete;
        AlgorithmRegistry& operator=(AlgorithmRegistry&&) = delete;

        void register_algorithm(AlgorithmEntry entry);
        void unregister_algorithm(AlgorithmEntry entry);

        std::unique_ptr<ISimpleAlgorithm> create_algorithm(
            const AlgorithmType& type
        ) const;
        std::unique_ptr<IAlgorithmVisualizer> create_visualizer(
            const AlgorithmType& type,
            const VisualizationConfig& visualization_config = {}
        ) const;

        [[nodiscard]] bool has_algorithm(
            const AlgorithmType& type
        ) const noexcept;
        [[nodiscard]] const std::vector<AlgorithmType>
        get_available_algorithm_types() const;
        [[nodiscard]] const std::vector<std::string>
        get_available_algorithm_names() const;
        [[nodiscard]] const CategorizedAlgorithms
        get_available_categorized_algorithms() const;

    private:
        nlohmann::json fetch_available_algorithms_json() const;
        bool create_entry_for_available_algorithms();

        core::JsonConfigManager& m_json_config_manager;
        ui::managers::IconManager& m_icon_manager;

        mutable std::shared_mutex m_mutex;

        std::unordered_map<AlgorithmType, AlgorithmEntry> m_entries;
        std::unordered_map<std::string, AlgorithmType> m_name_to_type;
        std::vector<AlgorithmType> m_available_algorithm_types;
        std::vector<std::string> m_available_algorithms_names;
        CategorizedAlgorithms m_available_categorized_algorithms;

        bool m_is_valid                 {false};
    };
} // namespace c2l::algorithms




#endif //CODE2LOGIC_ALGORITHM_REGISTRY_HPP