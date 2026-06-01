#ifndef CODE2LOGIC_INSERTION_SORT_HPP
#define CODE2LOGIC_INSERTION_SORT_HPP

#include "algorithms/core/json_algorithm_base.hpp"
#include "core/json_config_manager/json_config_manager.hpp"
#include "algorithms/core/algorithm_step.hpp"

#include <vector>
#include <string>

namespace c2l::algorithms
{
    /**
     * @brief Insertion Sort algorithm with JSON-driven metadata
     *
     * This implementation is fully driven by JSON configuration.
     * All metadata, step types, and visualizations are loaded from JSON.
     * Insertion sort builds the final sorted array one element at a time
     * by repeatedly inserting the current element into its correct position.
     */
    class InsertionSort final : public JsonAlgorithmBase
    {
    public:
        explicit InsertionSort(core::JsonConfigManager& json_config_manager);
        ~InsertionSort() override = default;

        InsertionSort(const InsertionSort&) = delete;
        InsertionSort& operator=(const InsertionSort&) = delete;
        InsertionSort(InsertionSort&&) noexcept = delete;
        InsertionSort& operator=(InsertionSort&&) noexcept = delete;

        // ISimpleAlgorithm interface
        void generate_all_steps() override;
        void reset_state() override {};

    private:
        /**
         * @brief Internal state for Insertion Sort algorithm
         */
        struct InsertionSortState
        {
            std::vector<int> data;
            size_t outer_loop_index         {0};     // Current element being inserted (i)
            size_t inner_loop_index         {0};     // Current comparison position (j)
            int current_key                 {0};     // Value being inserted
            bool shifting_active            {false}; // Whether we're in shifting phase
            size_t sorted_elements          {0};     // Number of elements already sorted
            size_t comparisons              {0};     // Total comparisons performed
            size_t swaps                    {0};     // Total swaps/assignments performed
        };

        void push_step(
            const InsertionSortState& state,
            const std::string& operation_id
        );

        /**
         * @brief Convert internal state to AlgorithmStep
         */
        AlgorithmStep create_step_from_state(
            const InsertionSortState& state,
            const std::string& operation_id
        ) const;

        /**
         * @brief Populate metadata for a step
         */
        void populate_step_metadata(
            AlgorithmStep& step,
            const InsertionSortState& state,
            const std::string& operation_id
        ) const;

        // Performance tracking
        size_t m_total_comparisons          {0};
        size_t m_total_swaps                {0};

    };
} // namespace c2l::algorithms

#endif //CODE2LOGIC_INSERTION_SORT_HPP