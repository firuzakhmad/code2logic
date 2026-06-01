#ifndef CODE2LOGIC_HEAP_SORT_HPP
#define CODE2LOGIC_HEAP_SORT_HPP

#include "algorithms/core/json_algorithm_base.hpp"
#include "algorithms/core/algorithm_step.hpp"
#include <vector>
#include <string>
#include <unordered_map>

namespace c2l::algorithms
{
    /**
     * @brief Heap Sort algorithm with JSON-driven metadata
     * 
     * Heap sort is a comparison-based sorting algorithm that uses a binary heap
     * data structure. It works by first building a max heap from the input array,
     * then repeatedly extracting the maximum element (root) and placing it at
     * the end of the array, reducing the heap size each time.
     * 
     * Key characteristics:
     * - Time Complexity: O(n log n) in all cases
     * - Space Complexity: O(1) (in-place)
     * - Not stable
     * - Not adaptive
     * - Guaranteed O(n log n) performance
     * - Excellent for systems with limited memory
     * 
     * This implementation uses an iterative heapify approach to avoid recursion
     * and includes detailed step-by-step visualization of both heap construction
     * and extraction phases.
     */
    class HeapSort final : public JsonAlgorithmBase
    {
    public:
        explicit HeapSort(core::JsonConfigManager& json_config_manager);
        ~HeapSort() override = default;

        // Disable copy, enable move
        HeapSort(const HeapSort&) = delete;
        HeapSort& operator=(const HeapSort&) = delete;
        HeapSort(HeapSort&&) noexcept = delete;
        HeapSort& operator=(HeapSort&&) noexcept = delete;

        void generate_all_steps() override;
        void reset_state() override {};

    private:
        /**
         * @brief Represents the current phase of the algorithm
         */
        enum class Phase
        {
            BUILD,          // Building initial max heap
            EXTRACTION      // Extracting elements to sorted portion
        };

        /**
         * @brief Internal state for Heap Sort algorithm
         * 
         * Tracks all necessary variables for algorithm visualization
         * and step-by-step execution, including heap-specific indices.
         */
        struct State
        {
            std::vector<int> data;                      // Current array state
            size_t heap_size        {0};                // Current size of the heap (unsorted portion)
            size_t root             {0};                // Current root index being heapified
            size_t largest          {0};                // Index of largest element among root and children
            size_t left             {0};                // Left child index
            size_t right            {0};                // Right child index
            size_t i                {0};                // Current loop index (build or extraction)
            size_t extract_index    {0};                // Current extraction index
            size_t build_heap_index {0};                // Current index during heap construction
            size_t comparisons      {0};                // Total comparisons performed
            size_t swaps            {0};                // Total swaps performed
            Phase phase             {Phase::BUILD};     // Current algorithm phase
        };

        /**
         * @brief Build max heap from unsorted array using Floyd's algorithm
         * 
         * @param state Current algorithm state (will be modified)
         */
        void build_heap(State& state);

        /**
         * @brief Perform iterative heapify operation on a subtree
         * 
         * @param state Current algorithm state
         * @param root Root index of the subtree to heapify
         */
        void heapify(State& state, size_t root);

        /**
         * @brief Push a new step to the steps vector
         * 
         * @param state Current algorithm state
         * @param operation_id Operation type identifier (maps to JSON step_mappings)
         */
        void push_step(
            const State& state, 
            const std::string& operation_id
        );

        /**
         * @brief Convert internal state to AlgorithmStep object
         * 
         * @param state Current algorithm state
         * @param operation_id Operation type identifier
         * @return Fully populated AlgorithmStep
         */
        AlgorithmStep create_step_from_state(
            const State& state, 
            const std::string& operation_id
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
         * This includes highlighting heap tree structure, showing parent-child
         * relationships, and marking sorted portions.
         * 
         * @param step Step to update
         * @param state Current algorithm state
         * @param operation_id Operation type identifier
         */
        void update_visualization_data(
            AlgorithmStep& step, 
            const State& state, 
            const std::string& operation_id
        ) const;

        /**
         * @brief Convert phase enum to string for metadata
         */
        std::string phase_to_string(Phase phase) const;

        /**
         * @brief Calculate child indices for a given parent
         */
        inline size_t left_child_index(size_t parent) const 
        { 
            return 2 * parent + 1; 
        }
        inline size_t right_child_index(size_t parent) const 
        { 
            return 2 * parent + 2; 
        }

        // Member variables
        size_t m_total_comparisons{0};             // Total comparisons across algorithm
        size_t m_total_swaps{0};                   // Total swaps across algorithm
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_HEAP_SORT_HPP