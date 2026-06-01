#ifndef CODE2LOGIC_MERGE_SORT_HPP
#define CODE2LOGIC_MERGE_SORT_HPP

#include "algorithms/core/json_algorithm_base.hpp"
#include "algorithms/core/algorithm_step.hpp"
#include <stack>
#include <queue>

namespace c2l::algorithms
{
    /**
     * @brief Merge Sort algorithm with JSON-driven metadata
     *
     * This implementation uses an iterative bottom-up approach
     * with explicit stack to avoid recursion depth limitations
     * and make step generation easier for visualization.
     * All metadata and visualizations are loaded from JSON.
     */
    class MergeSort final : public JsonAlgorithmBase
    {
    public:
        explicit MergeSort(core::JsonConfigManager& json_config_manager);
        ~MergeSort() override = default;

        // Disable copy, enable move
        MergeSort(const MergeSort&) = delete;
        MergeSort& operator=(const MergeSort&) = delete;
        MergeSort(MergeSort&&) noexcept = delete;
        MergeSort& operator=(MergeSort&&) noexcept = delete;

        // ISimpleAlgorithm Implementation
        void generate_all_steps() override;
        void reset_state() override {};

    private:
        /**
         * @brief Represents a subarray to be merged
         */
        struct MergeRange
        {
            size_t left;      // Start index of left subarray
            size_t mid;       // End index of left subarray (mid+1 is start of right)
            size_t right;     // End index of right subarray
            size_t depth;     // Recursion/merge depth

            MergeRange(
                const size_t l,
                const size_t m,
                const size_t r,
                const size_t d = 0
            )
                : left(l), mid(m), right(r), depth(d) {}
        };

        /**
         * @brief State for a single merge operation step
         */
        struct MergeStep
        {
            size_t left_index   {0};        // Current position in left subarray
            size_t right_index  {0};        // Current position in right subarray
            size_t target_index {0};        // Current position in merged array
            bool left_taken     {false};    // Whether left element was taken
            bool right_taken    {false};    // Whether right element was taken
        };

        /**
         * @brief Complete state of Merge Sort algorithm
         */
        struct MergeSortState
        {
            std::vector<int> data;                  // Current array state
            std::vector<int> aux;                   // Auxiliary array for merging
            std::queue<MergeRange> pending_merges;  // Merges to perform
            MergeRange current_merge{0, 0, 0, 0};   // Current merge operation
            MergeStep merge_step;                   // Current merge step details

            // Metrics
            size_t comparisons      {0};            // Total comparisons performed
            size_t copies           {0};            // Total element copies performed
            size_t max_depth        {0};            // Maximum merge depth
            size_t current_pass     {0};            // Current pass number (2^k size)
            size_t merge_size       {1};            // Current merge size (1, 2, 4, 8...)
            size_t merge_count      {0};            // Number of merges in current pass
            size_t merges_completed {0};            // Merges completed in current pass

            // Flags for state machine
            bool is_initializing    {true};         // In initialization phase
            bool is_merging         {false};        // Currently performing a merge
            bool is_complete        {false};        // Sort is complete

            // For visualization
            size_t highlighted_left     {0};        // Left element being compared
            size_t highlighted_right    {0};        // Right element being compared
            size_t highlighted_target   {0};        // Target position for copy
        };

        // Step Generation

        void merge_sort_recursive(
            MergeSortState &state,
            size_t left,
            size_t right,
            size_t depth
        );

        void merge(
            MergeSortState &state,
            size_t left,
            size_t mid,
            size_t right,
            size_t depth
        );

        void push_step(
            const MergeSortState& state,
            const std::string& operation_id
        );

        AlgorithmStep create_step_from_state(
            const MergeSortState& state,
            const std::string& operation_id
        ) const;

        void populate_step_metadata(
            AlgorithmStep& step,
            const MergeSortState& state,
            const std::string& operation_id
        ) const;

        void update_visualization_data(
            AlgorithmStep& step,
            const MergeSortState& state,
            const std::string& operation_id
        ) const;

        size_t m_total_comparisons{0};
        size_t m_total_copies{0};
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_MERGE_SORT_HPP