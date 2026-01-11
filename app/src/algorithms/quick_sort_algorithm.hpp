//
// Created by Akhmad on 11/8/25.
//

#ifndef CODE2LOGIC_QUICK_SORT_ALGORITHM_HPP
#define CODE2LOGIC_QUICK_SORT_ALGORITHM_HPP

#include "algorithm_base.hpp"
#include <stack>

namespace c2l::algorithms
{
    struct QuickSortState
    {
        size_t low                          {0};
        size_t high                         {0};
        std::optional<size_t> pivot_index;
        std::optional<size_t> pivot_value;
        std::optional<size_t> i;
        std::optional<size_t> j;
        std::optional<size_t> sorted_elements;
        bool is_partitioning                {false};
        bool is_swapping                    {false};
        bool is_recursive_call              {false};
        size_t recursion_depth              {0};

        [[nodiscard]] std::string to_string() const
        {
            std::string result;

            if (low && high)
            {
                result = "low=" + std::to_string(low) +
                             ", high=" + std::to_string(high);
            }

            if (pivot_index.has_value() && pivot_value.has_value()) {
                result += ", pivot=" + std::to_string(*pivot_value) +
                         " @[" + std::to_string(*pivot_index) + "]";
            }
            if (j.has_value()) {
                result += ", j=" + std::to_string(*j);
            }
            if (i.has_value()) {
                result += ", i=" + std::to_string(*i);
            }
            return result;
        }
    };

    struct Step
    {
        std::vector<int> data;
        QuickSortState state;
        std::string description;
        AlgorithmStepOperation operation_type;
        size_t total_comparisons    {0};
        size_t total_swaps          {0};
    };

    class QuickSortAlgorithm final : public AlgorithmBase
    {
    public:
        QuickSortAlgorithm();
        ~QuickSortAlgorithm() override = default;

        // ISimpleAlgorithm interface
        void initialize(const std::vector<int>& data) override;
        bool step_forward() override;
        bool step_backward() override;
        void reset() override;

        void populate_step_metadata(AlgorithmStep& out_step) const override;

        [[nodiscard]] std::vector<int> get_original_data() const override;
        [[nodiscard]] size_t get_step_count() const override;
        [[nodiscard]] size_t get_current_step_index() const override;
        [[nodiscard]] bool is_complete() const override;
        [[nodiscard]] bool is_steps_empty_or_invalid() const override;

        // Enhanced metadata
        [[nodiscard]] AlgorithmType get_type() const override;
        [[nodiscard]] std::string get_time_complexity() const override;
        [[nodiscard]] std::string get_space_complexity() const override;
        [[nodiscard]] std::string get_description() const override;
        [[nodiscard]] std::vector<std::string> get_detailed_description() const override;
        [[nodiscard]] std::unordered_map<std::string, std::string> get_properties() const override;
        [[nodiscard]] std::vector<std::string> get_variable_names() const override;

        // Algorithm-specific capabilities
        [[nodiscard]] bool supports_backward_steps() const override { return true; }
    
    private:
        void generate_all_steps();
        void quick_sort(
            std::vector<int>& data,
            size_t low,
            size_t high,
            size_t depth,
            size_t& comparisons,
            size_t& swaps
        );

        size_t partition(
            std::vector<int>& data,
            size_t low,
            size_t high,
            size_t depth,
            size_t& comparisons,
            size_t& swaps
        );

        void push_step(
            const std::vector<int>& data,
            const QuickSortState& state,
            AlgorithmStepOperation operation,
            std::string_view description,
            size_t comparisons,
            size_t swaps
        );

        std::vector<Step> m_steps;
    };
} // namespace c2l::algorithms

#endif //CODE2LOGIC_QUICK_SORT_ALGORITHM_HPP