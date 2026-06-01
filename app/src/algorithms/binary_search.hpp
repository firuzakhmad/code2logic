#ifndef CODE2LOGIC_BINARY_SEARCH_HPP
#define CODE2LOGIC_BINARY_SEARCH_HPP

#include "algorithms/core/json_algorithm_base.hpp"
#include "algorithms/core/algorithm_step.hpp"

#include <vector>
#include <string>
#include <optional>

namespace c2l::algorithms
{
    /**
     * @brief Binary Search algorithm with JSON-driven metadata
     * 
     * Binary search is an efficient algorithm for finding an element in a sorted array.
     * It works by repeatedly dividing the search interval in half and comparing the
     * middle element with the target value.
     * 
     * Key characteristics:
     * - Time Complexity: O(log n) in worst case
     * - Space Complexity: O(1) (iterative implementation)
     * - Requires sorted input array
     * - Much faster than linear search for large datasets
     * 
     * This implementation uses an iterative approach to avoid recursion overhead
     * and includes detailed step-by-step visualization of the search process.
     */
    class BinarySearch final : public JsonAlgorithmBase
    {
    public:
        explicit BinarySearch(
            core::JsonConfigManager& json_config_manager
        );
        ~BinarySearch() override = default;

        // Disable copy, enable move
        BinarySearch(const BinarySearch&) = delete;
        BinarySearch& operator=(const BinarySearch&) = delete;
        BinarySearch(BinarySearch&&) noexcept = delete;
        BinarySearch& operator=(BinarySearch&&) noexcept = delete;

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

    private:
        /**
         * @brief Internal state for Binary Search algorithm
         *
         * Tracks all necessary variables for algorithm visualization
         * and step-by-step execution.
         */
        struct State
        {
            std::vector<int> data;          // Current array state (unchanged during search)
            int target          {0};        // Target value being searched for
            size_t left         {0};        // Left boundary (inclusive)
            size_t right        {0};        // Right boundary (inclusive)
            size_t mid          {0};        // Middle index
            int mid_value       {0};        // Value at middle index
            size_t comparisons  {0};        // Total comparisons performed
            int result_index    {-1};       // Result index (-1 if not found)
            bool found          {false};    // Whether target was found
            size_t iteration    {0};        // Current iteration number
        };



        /**
         * @brief Perform the binary search and generate steps
         *
         * @param state Current algorithm state (will be modified)
         * @param searched_indices Track of searched indices
         * @param eliminated_indices Track of eliminated indices
         */
        void perform_search(
            State& state,
           std::vector<size_t>& searched_indices,
           std::vector<size_t>& eliminated_indices
        );

        /**
         * @brief Push a new step to the steps vector
         *
         * @param state Current algorithm state
         * @param operation_id Operation type identifier (maps to JSON step_mappings)
         * @param searched_indices Track of searched indices for this step
         * @param eliminated_indices Track of eliminated indices for this step
         */
        void push_step(
            const State& state,
            const std::string& operation_id,
            const std::vector<size_t>& searched_indices,
            const std::vector<size_t>& eliminated_indices
        );

        /**
         * @brief Convert internal state to AlgorithmStep object
         *
         * @param state Current algorithm state
         * @param operation_id Operation type identifier
         * @param searched_indices Track of searched indices for this step
         * @param eliminated_indices Track of eliminated indices for this step
         * @return Fully populated AlgorithmStep
         */
        AlgorithmStep create_step_from_state(
            const State& state,
            const std::string& operation_id,
            const std::vector<size_t>& searched_indices,
            const std::vector<size_t>& eliminated_indices
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
         * This includes highlighting boundaries, mid point, and searched regions.
         *
         * @param step Step to update
         * @param state Current algorithm state
         * @param operation_id Operation type identifier
         * @param searched_indices Track of searched indices for this step
         * @param eliminated_indices Track of eliminated indices for this step
         */
        void update_visualization_data(
            AlgorithmStep& step,
            const State& state,
            const std::string& operation_id,
            const std::vector<size_t>& searched_indices,
            const std::vector<size_t>& eliminated_indices
        ) const;

        /**
         * @brief Safely calculate mid to avoid overflow
         */
        inline size_t calculate_mid(
            size_t left,
            size_t right
        ) const
        {
            return left + (right - left) / 2;
        }

        /**
         * @brief Get interval size
         */
        inline size_t interval_size(
            size_t left,
            size_t right
        ) const
        {
            return right - left + 1;
        }

        // Member variables
        int m_target                    {0};        // Target value to search for
        bool m_found                    {false};
        int m_result_index              {-1};       // Result of the search
        size_t m_total_comparisons      {0};        // Total comparisons across algorithm
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_BINARY_SEARCH_HPP