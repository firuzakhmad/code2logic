#ifndef CODE2LOGIC_SELECTION_SORT_HPP
#define CODE2LOGIC_SELECTION_SORT_HPP

#include "algorithms/core/json_algorithm_base.hpp"
#include "algorithms/core/algorithm_step.hpp"
#include <vector>
#include <string>

namespace c2l::algorithms
{
    /**
     * @brief Selection Sort algorithm with JSON-driven metadata
     * 
     * Selection sort is an in-place comparison sorting algorithm that divides
     * the input list into a sorted and an unsorted region. It repeatedly selects
     * the minimum element from the unsorted region and swaps it with the first
     * element of the unsorted region.
     * 
     * Key characteristics:
     * - Time Complexity: O(n²) in all cases
     * - Space Complexity: O(1)
     * - Not stable
     * - Not adaptive
     * - Minimizes number of swaps (at most n-1)
     * 
     * This implementation is fully driven by JSON configuration.
     * All metadata, step types, and visualizations are loaded from JSON.
     */
    class SelectionSort final : public JsonAlgorithmBase
    {
    public:
        explicit SelectionSort(core::JsonConfigManager& json_config_manager);
        ~SelectionSort() override = default;

        // Disable copy, allow move
        SelectionSort(const SelectionSort&) = delete;
        SelectionSort& operator=(const SelectionSort&) = delete;
        SelectionSort(SelectionSort&&) noexcept = delete;
        SelectionSort& operator=(SelectionSort&&) noexcept = delete;

        // ISimpleAlgorithm Interface Implementation
        void initialize(const std::vector<int>& data) override;
        bool step_forward() override;
        bool step_backward() override;
        void reset() override;

        [[nodiscard]] std::vector<int> get_original_data() const override;
        [[nodiscard]] AlgorithmStep get_current_step() const override;
        [[nodiscard]] size_t get_step_count() const override;
        [[nodiscard]] size_t get_current_step_index() const override;
        [[nodiscard]] bool is_complete() const override;
        [[nodiscard]] bool is_steps_empty_or_invalid() const override;

    private:
        /**
         * @brief Internal state for Selection Sort algorithm
         * 
         * Tracks all necessary variables for algorithm visualization
         * and step-by-step execution.
         */
        struct SelectionSortState
        {
            std::vector<int> data;       	// Current array state
            size_t boundary_idx		{0};  	// Current boundary index (i)
            size_t scan_idx			{0};  	// Current scanning index (j)
            size_t min_idx			{0};  	// Index of current minimum found
            int min_value			{0};  	// Current minimum value (cached for display)
            size_t sorted_count		{0};  	// Number of sorted elements
            size_t comparisons		{0};   	// Total comparisons performed
            size_t swaps			{0};  	// Total swaps performed
            
            // Additional state for visualization clarity
            bool is_scanning			{false};	// Whether currently in scanning phase
            bool found_new_min			{false}; 	// Whether new minimum was found in current comparison
            size_t last_compared_idx	{0};       	// Last index that was compared
        };

        /**
         * @brief Generate all algorithm steps upfront for navigation
         * 
         * This method pre-computes every step of the selection sort algorithm
         * to enable forward/backward navigation through the visualization.
         * It follows the exact algorithm logic while capturing state at each
         * significant operation.
         */
        void generate_all_steps();

        /**
         * @brief Push a new step to the steps vector
         * 
         * @param data Current array state
         * @param state Current algorithm state
         * @param operation_id Operation type identifier (maps to JSON step_mappings)
         * @param total_comparisons Total comparisons so far
         * @param total_swaps Total swaps so far
         * @param additional_context Optional extra context for specific steps
         */
        void push_step(
            const std::vector<int>& data,
            const SelectionSortState& state,
            const std::string& operation_id,
            size_t total_comparisons,
            size_t total_swaps,
            const std::unordered_map<std::string, std::string>& additional_context = {}
        );

        /**
         * @brief Convert internal state to AlgorithmStep object
         * 
         * @param data Current array state
         * @param state Current algorithm state
         * @param operation_id Operation type identifier
         * @param total_comparisons Total comparisons so far
         * @param total_swaps Total swaps so far
         * @param additional_context Extra context for metadata
         * @return Fully populated AlgorithmStep
         */
        AlgorithmStep create_step_from_state(
            const std::vector<int>& data,
            const SelectionSortState& state,
            const std::string& operation_id,
            size_t total_comparisons,
            size_t total_swaps,
            const std::unordered_map<std::string, std::string>& additional_context = {}
        ) const;

        /**
         * @brief Populate metadata for a step using JSON templates
         * 
         * @param step Step to populate
         * @param state Current algorithm state
         * @param operation_id Operation type identifier
         * @param additional_context Extra context for specific metadata fields
         */
        void populate_step_metadata(
            AlgorithmStep& step,
            const SelectionSortState& state,
            const std::string& operation_id,
            const std::unordered_map<std::string, std::string>& additional_context = {}
        ) const;

        /**
         * @brief Update visualization-specific data for the step
         * 
         * @param step Step to update
         * @param state Current algorithm state
         * @param operation_id Operation type identifier
         */
        void update_visualization_data(
            AlgorithmStep& step,
            const SelectionSortState& state,
            const std::string& operation_id
        ) const;

        /**
         * @brief Create a comparison step with detailed context
         * 
         * Helper method to create consistent comparison steps
         */
        void add_comparison_step(
            const std::vector<int>& data,
            SelectionSortState& state,
            size_t j,
            size_t min_idx
        );

        // Member variables
        std::vector<int> m_original_data;          
        std::vector<AlgorithmStep> m_steps;        
        size_t m_current_step_index{0};            
        
        // Performance tracking
        size_t m_total_comparisons{0};              
        size_t m_total_swaps{0};                    
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_SELECTION_SORT_HPP