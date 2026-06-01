#ifndef CODE2LOGIC_INTERPOLATION_SEARCH_HPP
#define CODE2LOGIC_INTERPOLATION_SEARCH_HPP

#include "algorithms/core/json_algorithm_base.hpp"
#include "algorithms/core/algorithm_step.hpp"

#include <vector>
#include <string>
#include <optional>
#include <cmath>

namespace c2l::algorithms
{
    /**
     * @brief Interpolation Search algorithm with JSON-driven metadata
     *
     * Interpolation search is an improved variant of binary search that uses
     * the value distribution to estimate the position of the target.
     * It works best on uniformly distributed sorted data.
     *
     * Key characteristics:
     * - Time Complexity: O(log log n) average case, O(n) worst case
     * - Space Complexity: O(1) (iterative implementation)
     * - Requires sorted input array with uniform distribution
     * - Uses formula: pos = low + ((target - arr[low]) * (high - low)) / (arr[high] - arr[low])
     * - Faster than binary search for uniformly distributed data
     *
     * This implementation includes detailed step-by-step visualization
     * with position estimation, interpolation formulas, and performance metrics.
     */
    class InterpolationSearch final : public JsonAlgorithmBase
    {
    public:
        explicit InterpolationSearch(core::JsonConfigManager& json_config_manager);
        ~InterpolationSearch() override = default;

        // Disable copy, enable move
        InterpolationSearch(const InterpolationSearch&) = delete;
        InterpolationSearch& operator=(const InterpolationSearch&) = delete;
        InterpolationSearch(InterpolationSearch&&) noexcept = delete;
        InterpolationSearch& operator=(InterpolationSearch&&) noexcept = delete;

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
            size_t total_comparisons        {0};
            size_t iterations               {0};
            double average_guess_accuracy   {0.0};
            bool found                      {false};
            int result_index                {-1};
            std::vector<size_t> guess_positions;
            std::vector<double> guess_accuracies;
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
            CALCULATING_POSITION,  // Calculating interpolation position
            COMPARING,             // Comparing guessed position with target
            NARROWING_LEFT,        // Target is left of guessed position
            NARROWING_RIGHT,       // Target is right of guessed position
            FOUND,                 // Target found
            NOT_FOUND,             // Target not found
            COMPLETE               // Search complete
        };

    private:
        /**
         * @brief Internal state for Interpolation Search algorithm
         *
         * Tracks all necessary variables for algorithm visualization
         * and step-by-step execution.
         */
        struct State
        {
            std::vector<int> data;      // Current array state
            int target          {0};    // Target value being searched for
            size_t left         {0};    // Left boundary
            size_t right        {0};    // Right boundary
            size_t pos          {0};    // Calculated position
            size_t comparisons  {0};    // Total comparisons performed
            size_t iterations   {0};    // Number of iterations
            int result_index    {-1};   // Result index (-1 if not found)
            bool found          {false}; // Whether target was found
            SearchPhase phase   {SearchPhase::CALCULATING_POSITION};

            // Interpolation-specific calculations
            int left_value                      {0};
            int right_value                     {0};
            double interpolation_ratio          {0.0};
            bool prevented_overflow             {false};
            bool uniform_distribution_assumed   {true};

            // Performance tracking
            std::vector<size_t> guess_positions;
            std::vector<double> guess_accuracies;
        };

        /**
         * @brief Perform the interpolation search and generate steps
         */
        void perform_search(
            State& state,
            std::vector<size_t>& probed_indices,
            std::vector<size_t>& eliminated_indices);

        /**
         * @brief Calculate interpolation position
         */
        size_t calculate_position(
            const State& state
        ) const;

        /**
         * @brief Calculate interpolation ratio for visualization
         */
        double calculate_interpolation_ratio(
            const State& state
        ) const;

        /**
         * @brief Calculate guess accuracy
         */
        double calculate_guess_accuracy(
            const State& state
        ) const;
        
        /**
         * @brief Push a new step to the steps vector
         */
        void push_step(
            const State& state,
            const std::string& operation_id,
            const std::vector<size_t>& probed_indices,
            const std::vector<size_t>& eliminated_indices);

        /**
         * @brief Convert internal state to AlgorithmStep object
         */
        AlgorithmStep create_step_from_state(
            const State& state,
            const std::string& operation_id,
            const std::vector<size_t>& probed_indices,
            const std::vector<size_t>& eliminated_indices
        ) const;

        /**
         * @brief Populate metadata for a step using JSON templates
         */
        void populate_step_metadata(
            AlgorithmStep& step,
            const State& state,
            const std::string& operation_id
        ) const;

        /**
         * @brief Update visualization-specific data for the step
         */
        void update_visualization_data(
            AlgorithmStep& step,
            const State& state,
            const std::string& operation_id,
            const std::vector<size_t>& probed_indices,
            const std::vector<size_t>& eliminated_indices
        ) const;

        /**
         * @brief Calculate value distribution uniformity
         */
        double calculate_uniformity(
            const std::vector<int>& data
        ) const;

        // Member variables
        int m_target{0};                           // Target value to search for
        int m_result_index{-1};                    // Result of the search
        PerformanceMetrics m_metrics;              // Performance metrics
        double m_data_uniformity{1.0};             // Uniformity of data distribution
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_INTERPOLATION_SEARCH_HPP