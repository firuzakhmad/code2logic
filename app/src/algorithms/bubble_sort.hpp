//
// Created by Akhmad on 11/7/25.
//

#ifndef CODE2LOGIC_BUBBLE_SORT_ALGORITHM_HPP
#define CODE2LOGIC_BUBBLE_SORT_ALGORITHM_HPP

#include "algorithms/core/json_algorithm_base.hpp"
#include "algorithms/core/algorithm_state_tracker.hpp"
#include "algorithms/core/algorithm_step_operation.hpp"
#include "core/json_config_manager/json_config_manager.hpp"

namespace c2l::algorithms
{
    /**
     * @brief Bubble Sort algorithm with JSON-driven metadata
     * 
     * This implementation is fully driven by JSON configuration.
     * All metadata, step types, and visualizations are loaded from JSON.
     */
    class BubbleSort final : public JsonAlgorithmBase
    {
    public:
        explicit BubbleSort(core::JsonConfigManager& json_config_manager);
        ~BubbleSort() override = default;

        BubbleSort(const BubbleSort&) = delete;
        BubbleSort& operator=(const BubbleSort&) = delete;
        BubbleSort(BubbleSort&&) noexcept = delete;
        BubbleSort& operator=(BubbleSort&&) noexcept = delete;

        // ISimpleAlgorithm interface
        void initialize(const std::vector<int>& data) override;
        bool step_forward() override;
        bool step_backward() override;
        void reset() override;

        [[nodiscard]] std::vector<int> get_original_data() const override;
        [[nodiscard]] AlgorithmStep get_current_step() const override;
        [[nodiscard]] size_t get_current_step_index() const override;
        [[nodiscard]] size_t get_step_count() const override;
        [[nodiscard]] bool is_complete() const override;
        [[nodiscard]] bool is_steps_empty_or_invalid() const override;

    private:
        /**
         * @brief Internal state for Bubble Sort algorithm
         */
        struct BubbleSortState
        {
            std::vector<int> data;
            size_t outer_loop_index         {0};
            size_t inner_loop_index         {0};
            bool swapped_in_current_pass    {false};
            size_t sorted_elements          {0};
            size_t comparisons              {0};
            size_t swaps                    {0};
        };

        void generate_all_steps();

        void push_step(
            const std::vector<int>& data,
            const BubbleSortState& state,
            const std::string& operation_id,
            size_t total_comparisons,
            size_t total_swaps
        );

        /**
         * @brief Convert internal state to AlgorithmStep
         */
        AlgorithmStep create_step_from_state(
            const std::vector<int>& data,
            const BubbleSortState& state,
            const std::string& operation_id,
            size_t total_comparisons,
            size_t total_swaps
        ) const;

        /**
         * @brief Populate metadata for a step
         */
        void populate_step_metadata(
            AlgorithmStep& step,
            const BubbleSortState& state,
            const std::string& operation_id,
            size_t total_comparisons,
            size_t total_swaps
        ) const;

        std::vector<int> m_original_data;
        std::vector<AlgorithmStep> m_steps;
        size_t m_current_step_index         {0};

        // Performance tracking
        size_t m_total_comparisons          {0};
        size_t m_total_swaps                {0};

    };
} // namespace c2l::algorithms

#endif //CODE2LOGIC_BUBBLE_SORT_ALGORITHM_HPP