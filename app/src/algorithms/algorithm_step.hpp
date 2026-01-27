//
// Created by Akhmad on 1/2/26.
//

#ifndef CODE2LOGIC_ALGORITHM_STEP_HPP
#define CODE2LOGIC_ALGORITHM_STEP_HPP

#include "algorithms/algorithm_variable.hpp"
#include "algorithm_step_operation.hpp"
#include "core/utils/logger/logger.hpp"

#include <vector>
#include <string>
#include <unordered_map>
#include <optional>

namespace c2l::algorithms
{
    struct AlgorithmStep
    {
        std::vector<int> data;
        std::string description;

        struct MetaData
        {
            std::unordered_map<std::string, AlgorithmVariable> variables;
            std::unordered_map<std::string, std::string> tags;
            std::vector<std::string> notes;
            AlgorithmStepOperation operation_type{ AlgorithmStepOperation::NONE };

            // Type safe variable access
            template<typename T>
            [[nodiscard]] std::optional<T> get(const std::string& key) const
            {
                const auto it = variables.find(key);
                if (it == variables.end())
                {
                    LOG_ERROR("Could not get value {}", key);
                    return std::nullopt;
                }

                return it->second.template get<T>();
            }

            [[nodiscard]] std::optional<AlgorithmVariable> get_variable(const std::string& key) const
            {
                const auto it = variables.find(key);
                if (it == variables.end())
                {
                    LOG_ERROR("Could not find variable {}", key);
                    return std::nullopt;
                }

                return std::make_optional(it->second);
            }

            template<typename T>
            void set(const std::string& name,
                     const T& value,
                     const std::string& display_name = "")
            {
                variables.emplace(name, AlgorithmVariable{name, value, display_name});
            }

            void set_variable(const AlgorithmVariable& variable)
            {
                variables[variable.get_name()] = variable;
            }

            void add_tag(const std::string& tag)    { tags[tag] = tag; }
            void add_note(const std::string& note)  { notes.push_back(note); }

            [[nodiscard]] bool has_tag(const std::string& tag) const
            {
                return tags.find(tag) != tags.end();
            }

        } metadata;


        // Visualization data
        struct VisualizationData
        {
            // Array-based algorithms
            std::optional<size_t> highlighted_index;
            std::optional<size_t> compared_index;
            std::vector<size_t> additional_highlights;
            bool is_partition_step      {false};
            bool is_swap_step           {false};

            // Graph-based algorithms
            struct GraphState
            {
                std::vector<size_t> visited_nodes;
                std::vector<size_t> frontier_nodes;
                std::vector<std::pair<size_t, size_t>> active_edges;
                std::vector<size_t> path;
                std::unordered_map<size_t, int> node_distances;
                std::unordered_map<size_t, size_t> node_parents;
            } graph_state;

            // Tree-based algorithms
            struct TreeNode
            {
                int value;
                std::optional<size_t> left_child;
                std::optional<size_t> right_child;
                std::optional<size_t> parent;
                int height              {0};
                int balance_factor      {0};
            };
            std::vector<TreeNode> tree_nodes;
            size_t tree_root            {0};

            // Performance metrics
            size_t comparisons          {0};
            size_t swaps                {0};
            size_t memory_usage         {0};
        } visualization;

        AlgorithmStep() = default;
        AlgorithmStep(std::vector<int> data_vec, std::string desc)
            : data(std::move(data_vec)), description(std::move(desc)) {}

    };
} // namespace c2l::algorithms

#endif //CODE2LOGIC_ALGORITHM_STEP_HPP