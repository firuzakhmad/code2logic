#ifndef CODE2LOGIC_JUMP_SEARCH_HPP
#define CODE2LOGIC_JUMP_SEARCH_HPP

#include "algorithms/core/json_algorithm_base.hpp"
#include "algorithms/core/algorithm_step.hpp"

#include <vector>
#include <string>
#include <optional>
#include <cmath>

namespace c2l::algorithms
{
    /**
     * @brief Jump Search algorithm with JSON-driven metadata
     *
     * Jump Search (also known as Block Search) is an efficient searching algorithm
     * for sorted arrays that works by jumping ahead by fixed steps (optimal √n)
     * and then performing linear search within the identified block.
     *
     * Key characteristics:
     * - Time Complexity: O(√n) in all cases
     * - Space Complexity: O(1) (iterative implementation)
     * - Requires sorted input array
     * - Better cache locality than binary search for large arrays
     * - Optimal block size is √n (mathematically proven)
     *
     * This implementation includes detailed step-by-step visualization
     * with jump animations, block highlighting, and performance metrics.
     */
    class JumpSearch final : public JsonAlgorithmBase
    {
    public:
        explicit JumpSearch(core::JsonConfigManager& json_config_manager);
        ~JumpSearch() override = default;

        // Disable copy, enable move
        JumpSearch(const JumpSearch&) = delete;
        JumpSearch& operator=(const JumpSearch&) = delete;
        JumpSearch(JumpSearch&&) noexcept = delete;
        JumpSearch& operator=(JumpSearch&&) noexcept = delete;

        // ISimpleAlgorithm Interface Implementation
        void initialize(const std::vector<int>& data) override;
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
            size_t jumps_performed      {0};
            size_t block_size           {0};
            double optimal_block_size   {0.0};
            bool found                  {false};
            int result_index            {-1};
            size_t linear_scans         {0};
        };

        [[nodiscard]] PerformanceMetrics get_metrics() const 
        { 
            return m_metrics; 
        }

        /**
         * @brief Search phases for visualization
         */
        enum class SearchPhase
        {
            JUMPING,        // Jumping through blocks
            BLOCK_FOUND,    // Found target block
            LINEAR_SCAN,    // Performing linear scan in block
            COMPLETE        // Search complete
        };

    private:
        /**
         * @brief Internal state for Jump Search algorithm
         *
         * Tracks all necessary variables for algorithm visualization
         * and step-by-step execution.
         */
        struct State
        {
            std::vector<int> data;              // Current array state
            int target              {0};        // Target value being searched for
            size_t step             {0};        // Current jump step size
            size_t prev             {0};        // Previous block boundary
            size_t current_index    {0};        // Current index (for linear scan)
            size_t block_start      {0};        // Start of current block
            size_t block_end        {0};        // End of current block
            size_t comparisons      {0};        // Total comparisons performed
            size_t jumps_performed  {0};        // Number of jumps made
            size_t linear_scans     {0};        // Number of linear comparisons
            int result_index        {-1};       // Result index (-1 if not found)
            bool found              {false};    // Whether target was found
            SearchPhase phase       {SearchPhase::JUMPING};  // Current search phase
            size_t iteration        {0};        // Current iteration number
            float search_progress   {0.0f};     // Progress through array
        };

        /**
         * @brief Perform the jump search and generate steps
         *
         * @param state Current algorithm state (will be modified)
         * @param jumped_indices Track of jumped-to indices
         * @param scanned_indices Track of linearly scanned indices
         * @param block_indices Track of block boundaries
         */
        void perform_search(
            State& state,
            std::vector<size_t>& jumped_indices,
            std::vector<size_t>& scanned_indices,
            std::vector<std::pair<size_t, size_t>>& block_indices);

        /**
         * @brief Calculate optimal block size for array
         */
        inline size_t calculate_block_size(size_t n) const
        {
            if (n == 0) return 1;
            return static_cast<size_t>(std::sqrt(static_cast<double>(n)));
        }

        /**
         * @brief Push a new step to the steps vector
         *
         * @param state Current algorithm state
         * @param operation_id Operation type identifier (maps to JSON step_mappings)
         * @param jumped_indices Track of jumped-to indices for this step
         * @param scanned_indices Track of linearly scanned indices for this step
         * @param block_indices Track of block boundaries for this step
         */
        void push_step(
            const State& state,
            const std::string& operation_id,
            const std::vector<size_t>& jumped_indices,
            const std::vector<size_t>& scanned_indices,
            const std::vector<std::pair<size_t, size_t>>& block_indices
        );

        /**
         * @brief Convert internal state to AlgorithmStep object
         *
         * @param state Current algorithm state
         * @param operation_id Operation type identifier
         * @param jumped_indices Track of jumped-to indices for this step
         * @param scanned_indices Track of linearly scanned indices for this step
         * @param block_indices Track of block boundaries for this step
         * @return Fully populated AlgorithmStep
         */
        AlgorithmStep create_step_from_state(
            const State& state,
            const std::string& operation_id,
            const std::vector<size_t>& jumped_indices,
            const std::vector<size_t>& scanned_indices,
            const std::vector<std::pair<size_t, size_t>>& block_indices
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
         * This includes highlighting jump positions, block boundaries,
         * linear scan positions, and search progress indicators.
         *
         * @param step Step to update
         * @param state Current algorithm state
         * @param operation_id Operation type identifier
         * @param jumped_indices Track of jumped-to indices for this step
         * @param scanned_indices Track of linearly scanned indices for this step
         * @param block_indices Track of block boundaries for this step
         */
        void update_visualization_data(
            AlgorithmStep& step,
            const State& state,
            const std::string& operation_id,
            const std::vector<size_t>& jumped_indices,
            const std::vector<size_t>& scanned_indices,
            const std::vector<std::pair<size_t, size_t>>& block_indices
        ) const;

        // Member variables
        int m_target{0};                           // Target value to search for
        int m_result_index{-1};                    // Result of the search
        PerformanceMetrics m_metrics;              // Performance metrics
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_JUMP_SEARCH_HPP