//
// Created by Akhmad on 1/2/26.
//

#ifndef CODE2LOGIC_ALGORITHM_STEP_HPP
#define CODE2LOGIC_ALGORITHM_STEP_HPP

#include "algorithms/core/algorithm_variable.hpp"
#include "algorithms/core/algorithm_step_operation.hpp"
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
            std::string operation_id;

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
            bool is_complete            {false};


            // Quick Sort specific
            size_t subarray_low{0};
            size_t subarray_high{0};
            size_t recursion_depth{0};

            // Heap sort
            size_t heap_size{0};
            size_t heapify_root{0};
            std::vector<size_t> heap_structure;
            
            // Partition boundaries visualization
            std::vector<size_t> less_than_pivot_indices;
            std::vector<size_t> greater_than_pivot_indices;

            // Merge Sort specific
            size_t merge_boundary{0};           // Mid point between left and right subarrays
            bool is_merge_step{false};          // Flag for merge operations
            bool is_compare_step{false};


            // Search-specific fields
            struct SearchVisualization
            {
                int target_value{0};                        // Value being searched for
                std::vector<size_t> searched_indices;       // Indices that have been examined
                std::vector<size_t> eliminated_indices;     // Indices eliminated from search
                std::optional<size_t> found_index;          // Where target was found
                bool is_searching{true};                    // Whether search is still active
                size_t search_step{0};                      // Current search step number
                float search_progress{0.0f};                // Progress through search

                // For binary search specifically
                std::optional<size_t> left_boundary;
                std::optional<size_t> right_boundary;
                std::optional<size_t> mid_point;
            } search;

            // For backward compatibility with existing code
            std::vector<size_t> eliminated_regions;  // Can map to search.eliminated_indices
            int target_value{0};                     // Can map to search.target_value
            bool is_found{false};                    // Can map to search.found_index.has_value()


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

            bool is_merge_complete{false};
            bool is_insertion_step{false};
            bool is_shift_step{false};

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
            size_t comparison_count          {0};
            size_t swap_count                {0};
            size_t visited_node_count        {0};
            size_t explored_node_count       {0};
            size_t memory_usage         {0};
        } visualization;
        
        AlgorithmStep() = default;
        AlgorithmStep(std::vector<int> data_vec, std::string desc)
            : data(std::move(data_vec)), description(std::move(desc)) {}

    };
} // namespace c2l::algorithms

#endif //CODE2LOGIC_ALGORITHM_STEP_HPP