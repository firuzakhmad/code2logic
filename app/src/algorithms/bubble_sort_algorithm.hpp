//
// Created by Akhmad on 11/7/25.
//

#ifndef CODE2LOGIC_BUBBLE_SORT_ALGORITHM_HPP
#define CODE2LOGIC_BUBBLE_SORT_ALGORITHM_HPP

#include "algorithms/algorithm_base.hpp"
#include "algorithms/algorithm_state_tracker.hpp"
#include "algorithms/algorithm_step_operation.hpp"

namespace c2l::algorithms
{
    struct BubbleSortState
    {
        size_t outer_loop_index         {0};
        size_t inner_loop_index         {0};
        bool swapped_in_current_pass    {false};
        size_t comparisons_in_pass      {0};
        size_t swaps_in_pass            {0};
        size_t sorted_elements          {0};

        [[nodiscard]] std::string to_string() const
        {
            return "i=" + std::to_string(outer_loop_index) +
                   ", j=" + std::to_string(inner_loop_index) +
                   ", swapped=" + std::to_string(swapped_in_current_pass);
        }
    };

    class BubbleSortAlgorithm final : public AlgorithmBase
    {
    public:
        BubbleSortAlgorithm();
        ~BubbleSortAlgorithm() override = default;

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
        struct Step
        {
            std::vector<int> data;
            BubbleSortState state;
            std::string description;
            size_t total_comparisons    {0};
            size_t total_swaps          {0};
            AlgorithmStepOperation operation_type;
        };

        void generate_all_steps();
        void push_step(
            const std::vector<int>& data,
            const BubbleSortState& state,
            AlgorithmStepOperation operatorn,
            std::string_view description,
            size_t total_comparison,
            size_t total_swaps);

        std::vector<Step> m_steps;
    };
} // namespace c2l::algorithms

#endif //CODE2LOGIC_BUBBLE_SORT_ALGORITHM_HPP