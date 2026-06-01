#ifndef CODE2LOGIC_LINEAR_SEARCH_HPP
#define CODE2LOGIC_LINEAR_SEARCH_HPP

#include "algorithms/core/json_algorithm_base.hpp"
#include "algorithms/core/algorithm_step.hpp"

#include <vector>
#include <string>
#include <optional>

namespace c2l::algorithms
{
    /**
     * @brief Linear Search algorithm with JSON-driven metadata
     *
     * Linear search is a simple searching algorithm that sequentially checks
     * each element in the array until the target is found or the array ends.
     *
     * Key characteristics:
     * - Time Complexity: O(n) in worst case, O(1) in best case
     * - Space Complexity: O(1) (iterative implementation)
     * - Works on both sorted and unsorted arrays
     * - Excellent for small datasets or single searches
     *
     * This implementation includes detailed step-by-step visualization
     * with progress tracking and performance metrics.
     */
    class LinearSearch final : public JsonAlgorithmBase
    {
    public:
        explicit LinearSearch(
            core::JsonConfigManager& json_config_manager
        );
        ~LinearSearch() override = default;

        // Disable copy, enable move
        LinearSearch(const LinearSearch&) = delete;
        LinearSearch& operator=(const LinearSearch&) = delete;
        LinearSearch(LinearSearch&&) noexcept = delete;
        LinearSearch& operator=(LinearSearch&&) noexcept = delete;

        // ISimpleAlgorithm Interface Implementation
        void generate_all_steps() override;
        void reset_state() override;

        /**
         * @brief Set the target value to search for
         * @param target The value to search for
         */
        void set_search_target(int target) override;

        /**
         * @brief Get the current target value
         * @return The target value being searched for
         */
        [[nodiscard]] int get_search_target() const noexcept override
        {
            return m_target;
        }

        /**
         * @brief Get the result index (where target was found or -1)
         */
        [[nodiscard]] int get_result_index() const 
        { 
            return m_result_index; 
        }

        /**
         * @brief Get performance metrics for the search
         */
        struct PerformanceMetrics
        {
            size_t total_comparisons    {0};
            size_t elements_checked     {0};
            double search_time_ms       {0.0};
            bool found                  {false};
            int result_index            {-1};
        };

        [[nodiscard]] PerformanceMetrics get_metrics() const 
        { 
            return m_metrics; 
        }

    private:
        /**
         * @brief Internal state for Linear Search algorithm
         *
         * Tracks all necessary variables for algorithm visualization
         * and step-by-step execution.
         */
        struct State
        {
            std::vector<int> data;              // Current array state
            int target              {0};        // Target value being searched for
            size_t current_index    {0};        // Current index being examined
            size_t comparisons      {0};        // Total comparisons performed
            size_t elements_checked {0};        // Number of elements checked
            int result_index        {-1};       // Result index (-1 if not found)
            bool found              {false};    // Whether target was found
            size_t iteration        {0};        // Current iteration number
            float search_progress   {0.0f};     // Progress through array
        };

        /**
         * @brief Perform the linear search and generate steps
         *
         * @param state Current algorithm state (will be modified)
         * @param checked_indices Track of checked indices
         */
        void perform_search(
            State& state, 
            std::vector<size_t>& checked_indices)
        ;

        /**
         * @brief Push a new step to the steps vector
         *
         * @param state Current algorithm state
         * @param operation_id Operation type identifier (maps to JSON step_mappings)
         * @param checked_indices Track of checked indices for this step
         */
        void push_step(
            const State& state,
            const std::string& operation_id,
            const std::vector<size_t>& checked_indices
        );

        /**
         * @brief Convert internal state to AlgorithmStep object
         *
         * @param state Current algorithm state
         * @param operation_id Operation type identifier
         * @param checked_indices Track of checked indices for this step
         * @return Fully populated AlgorithmStep
         */
        AlgorithmStep create_step_from_state(
            const State& state,
            const std::string& operation_id,
            const std::vector<size_t>& checked_indices
        ) const;

        /**
         * @brief Populate metadata for a step using JSON templates
         *
         * @param step Step to populate
         * @param state Current algorithm state
         * @param operation_id Operation type identifier
         */
        void populate_step_metadata(
            AlgorithmStep& step,
            const State& state,
            const std::string& operation_id
        ) const;

        /**
         * @brief Update visualization-specific data for the step
         *
         * This includes highlighting current element, checked regions,
         * and search progress indicators.
         *
         * @param step Step to update
         * @param state Current algorithm state
         * @param operation_id Operation type identifier
         * @param checked_indices Track of checked indices for this step
         */
        void update_visualization_data(
            AlgorithmStep& step,
            const State& state,
            const std::string& operation_id,
            const std::vector<size_t>& checked_indices
        ) const;

        // Member variables
        int m_target{0};                           // Target value to search for
        int m_result_index{-1};                    // Result of the search
        PerformanceMetrics m_metrics;              // Performance metrics
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_LINEAR_SEARCH_HPP