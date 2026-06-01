#ifndef CODE2LOGIC_QUICK_SORT_HPP
#define CODE2LOGIC_QUICK_SORT_HPP

#include "algorithms/core/json_algorithm_base.hpp"
#include "algorithms/core/algorithm_step.hpp"
#include <stack>

namespace c2l::algorithms
{
    /**
     * @brief Quick Sort algorithm with JSON-driven metadata
     * 
     * This implementation uses an iterative approach with explicit stack
     * to avoid recursion depth limitations and make step generation easier.
     * All metadata and visualizations are loaded from JSON.
     */
    class QuickSort final : public JsonAlgorithmBase
    {
    public:
        explicit QuickSort(core::JsonConfigManager& json_config_manager);
        ~QuickSort() override = default;

        // Disable copy, enable move
        QuickSort(const QuickSort&) = delete;
        QuickSort& operator=(const QuickSort&) = delete;
        QuickSort(QuickSort&&) noexcept = delete;
        QuickSort& operator=(QuickSort&&) noexcept = delete;

        // ISimpleAlgorithm Implementation
        void generate_all_steps() override;
        void reset_state() override {};

    private:
        // Internal Types
        struct Subarray
        {
            size_t low;
            size_t high;
            size_t depth;
        };

        struct PartitionResult
        {
            size_t pivot_index;
            size_t comparisons;
            size_t swaps;
        };

        struct QuickSortState
        {
            std::vector<int> data;
            std::stack<Subarray> stack;
            Subarray current_subarray{0, 0, 0};
            
            // Partitioning state
            size_t i{0};
            size_t j{0};
            int pivot{0};
            size_t pivot_index{0};
            
            // Metrics
            size_t comparisons{0};
            size_t swaps{0};
            size_t max_depth{0};
            
            // For tracking partition phase
            bool is_partitioning{false};
            size_t partition_low{0};
            size_t partition_high{0};
        };

        // Step Generation
        void push_step(
            const QuickSortState& state, 
            const std::string& operation_id
        );
        AlgorithmStep create_step_from_state(
            const QuickSortState& state, 
            const std::string& operation_id
        ) const;
        void populate_step_metadata(
            AlgorithmStep& step, 
            const QuickSortState& state, 
            const std::string& operation_id
        ) const;
        void update_visualization_data(
            AlgorithmStep& step, 
            const QuickSortState& state, 
            const std::string& operation_id
        ) const;

        // Partition Algorithm
        PartitionResult lomuto_partition(
            QuickSortState& state, 
            Subarray& subarray, 
            size_t& total_comparisons, 
            size_t& total_swaps
        );
        
        size_t m_total_comparisons{0};
        size_t m_total_swaps{0};
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_QUICK_SORT_HPP