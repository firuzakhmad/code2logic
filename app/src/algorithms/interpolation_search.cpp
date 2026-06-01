#include "interpolation_search.hpp"
#include "core/utils/logger/logger.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace c2l::algorithms
{
    InterpolationSearch::InterpolationSearch(core::JsonConfigManager& json_config_manager)
        : JsonAlgorithmBase(json_config_manager, AlgorithmType::INTERPOLATION_SEARCH)
    {
        LOG_DEBUG("InterpolationSearch created and metadata loaded from JSON");
    }

    void InterpolationSearch::initialize(const std::vector<int>& data)
    {
        m_original_data.clear();
        reset();

        if (data.empty())
        {
            LOG_WARNING("InterpolationSearch initialized with empty data");
            return;
        }

        m_original_data = data;

        // Verify array is sorted (interpolation search requirement)
        if (!std::is_sorted(data.begin(), data.end()))
        {
            LOG_WARNING(
                "InterpolationSearch requires sorted input array. Array is being sorted."
            );
            std::sort(m_original_data.begin(), m_original_data.end());
        }

        // Calculate data uniformity for performance prediction
        m_data_uniformity = calculate_uniformity(m_original_data);

        generate_all_steps();

        LOG_INFO(
            "InterpolationSearch initialized with {} elements. Data uniformity: {:.2f}%",
            data.size(),
            m_data_uniformity * 100.0
        );
    }

    void InterpolationSearch::set_search_target(const int target)
    {
        m_target = target;
        reset();
        generate_all_steps();
    }

    void InterpolationSearch::reset_state()
    {
        m_result_index = -1;
        m_metrics = PerformanceMetrics{};
    }

    void InterpolationSearch::generate_all_steps()
    {
        if (m_original_data.empty())
        {
            LOG_WARNING("Cannot generate steps: empty data");
            return;
        }

        m_steps.clear();
        m_metrics.total_comparisons = 0;
        m_metrics.iterations = 0;
        m_metrics.guess_positions.clear();
        m_metrics.guess_accuracies.clear();
        m_result_index = -1;

        // Initializing state
        State state;
        state.data = m_original_data;
        state.target = m_target;
        state.left = 0;
        state.right = m_original_data.size() - 1;
        state.left_value = m_original_data[0];
        state.right_value = m_original_data.back();
        state.comparisons = 0;
        state.iterations = 0;
        state.result_index = -1;
        state.found = false;
        state.phase = SearchPhase::CALCULATING_POSITION;
        state.uniform_distribution_assumed = m_data_uniformity > 0.7;

        // Tracking visualization data
        std::vector<size_t> probed_indices;
        std::vector<size_t> eliminated_indices;

        // Initialization step
        push_step(
            state, 
            "init", 
            probed_indices, 
            eliminated_indices
        );

        // Check if target is within range
        if (state.target < state.left_value || 
            state.target > state.right_value)
        {
            state.phase = SearchPhase::NOT_FOUND;
            push_step(
                state, 
                "out_of_range", 
                probed_indices, 
                eliminated_indices
            );
            push_step(
                state, 
                "completed", 
                probed_indices, 
                eliminated_indices
            );

            m_metrics.found = false;
            m_metrics.result_index = -1;
            return;
        }

        // Performing interpolation search
        perform_search(
            state, 
            probed_indices, 
            eliminated_indices
        );

        // Final completion step
        push_step(
            state, 
            "completed", 
            probed_indices, 
            eliminated_indices
        );

        // Update metrics
        m_metrics.total_comparisons = state.comparisons;
        m_metrics.iterations = state.iterations;
        m_metrics.found = state.found;
        m_metrics.result_index = state.result_index;
        m_metrics.guess_positions = state.guess_positions;
        m_metrics.guess_accuracies = state.guess_accuracies;

        LOG_DEBUG(
            "Generated {} steps for interpolation search (target: {}, uniformity: {:.2f}%)",
            m_steps.size(),
            m_target,
            m_data_uniformity * 100.0
        );
    }

    void InterpolationSearch::perform_search(
        State& state,
        std::vector<size_t>& probed_indices,
        std::vector<size_t>& eliminated_indices)
    {
        while (state.left <= state.right && !state.found)
        {
            state.iterations++;

            // Update boundary values
            state.left_value = state.data[state.left];
            state.right_value = state.data[state.right];

            // Step 1: Calculate interpolation position
            state.phase = SearchPhase::CALCULATING_POSITION;
            state.interpolation_ratio = calculate_interpolation_ratio(state);
            state.pos = calculate_position(state);
            state.prevented_overflow = (state.pos >= state.left && state.pos <= state.right);

            push_step(
                state, 
                "calculate_position", 
                probed_indices, 
                eliminated_indices
            );

            // Record guess for metrics
            state.guess_positions.push_back(state.pos);
            double accuracy = calculate_guess_accuracy(state);
            state.guess_accuracies.push_back(accuracy);

            // Record probed index
            probed_indices.push_back(state.pos);

            // Step 2: Compare with target
            state.phase = SearchPhase::COMPARING;
            state.comparisons++;
            push_step(
                state, 
                "compare", 
                probed_indices, 
                eliminated_indices
            );

            if (state.data[state.pos] == state.target)
            {
                // Target found
                state.found = true;
                state.result_index = static_cast<int>(state.pos);
                m_result_index = static_cast<int>(state.pos);
                state.phase = SearchPhase::FOUND;
                push_step(
                    state, 
                    "found", 
                    probed_indices, 
                    eliminated_indices
                );
                break;
            }

            // Step 3: Narrow search range
            if (state.data[state.pos] < state.target)
            {
                // Target is in the right subarray
                state.phase = SearchPhase::NARROWING_RIGHT;

                // Eliminate left portion including current position
                for (size_t i = state.left; i <= state.pos; ++i)
                {
                    if (std::find(
                        eliminated_indices.begin(), 
                        eliminated_indices.end(), i) == eliminated_indices.end())
                    {
                        eliminated_indices.push_back(i);
                    }
                }

                state.left = state.pos + 1;
                push_step(
                    state, 
                    "narrow_right", 
                    probed_indices, 
                    eliminated_indices
                );
            }
            else
            {
                // Target is in the left subarray
                state.phase = SearchPhase::NARROWING_LEFT;

                // Eliminate right portion including current position
                for (size_t i = state.pos; i <= state.right; ++i)
                {
                    if (std::find(
                        eliminated_indices.begin(), 
                        eliminated_indices.end(), i) == eliminated_indices.end())
                    {
                        eliminated_indices.push_back(i);
                    }
                }

                if (state.pos > 0)
                    state.right = state.pos - 1;
                else
                    break;

                push_step(
                    state, 
                    "narrow_left", 
                    probed_indices, 
                    eliminated_indices
                );
            }
        }

        // Target not found
        if (!state.found)
        {
            state.result_index = -1;
            m_result_index = -1;
            state.phase = SearchPhase::NOT_FOUND;
            push_step(
                state, 
                "not_found", 
                probed_indices, 
                eliminated_indices
            );
        }
    }

    void InterpolationSearch::push_step(
        const State& state,
        const std::string& operation_id,
        const std::vector<size_t>& probed_indices,
        const std::vector<size_t>& eliminated_indices)
    {
        auto step = create_step_from_state(
            state, 
            operation_id, 
            probed_indices, 
            eliminated_indices
        );
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep InterpolationSearch::create_step_from_state(
        const State& state,
        const std::string& operation_id,
        const std::vector<size_t>& probed_indices,
        const std::vector<size_t>& eliminated_indices
    ) const
    {
        AlgorithmStep step;

        // Core data
        step.data = state.data;
        step.metadata.operation_id = operation_id;

        // Populating metadata using JSON-driven approach
        populate_step_metadata(step, state, operation_id);

        // Generating description using JSON template
        step.description = format_step_description(operation_id, step);

        // Updating visualization data
        update_visualization_data(
            step, 
            state, 
            operation_id, 
            probed_indices, 
            eliminated_indices
        );

        return step;
    }

    void InterpolationSearch::populate_step_metadata(
        AlgorithmStep& step, const State& state,
        const std::string& operation_id) const
    {
        const size_t n = state.data.size();

        // Core search variables
        step.metadata.set(
            "target", 
            state.target, 
            "Target value being searched for"
        );
        step.metadata.set(
            "left", 
            state.left, 
            "Left boundary of search interval"
        );
        step.metadata.set(
            "right", 
            state.right, 
            "Right boundary of search interval"
        );
        step.metadata.set(
            "pos", 
            state.pos, 
            "Interpolated position estimate"
        );
        step.metadata.set(
            "left_value", 
            state.left_value, 
            "Value at left boundary"
        );
        step.metadata.set(
            "right_value", 
            state.right_value, 
            "Value at right boundary"
        );
        step.metadata.set(
            "comparisons", 
            state.comparisons, 
            "Total comparisons performed"
        );
        step.metadata.set(
            "iterations", 
            state.iterations, 
            "Number of iterations"
        );
        step.metadata.set(
            "found", 
            state.found, 
            "Whether target has been found"
        );
        step.metadata.set(
            "result_index", 
            state.result_index, 
            "Index where target was found (-1 if not found)"
        );
        step.metadata.set(
            "size", 
            n, 
            "Total array size"
        );
        step.metadata.set(
            "data_uniformity", 
            m_data_uniformity, 
            "Data distribution uniformity"
        );

        // Interpolation-specific metadata
        if (operation_id == "calculate_position" || 
            operation_id == "completed")
        {
            step.metadata.set(
                "interpolation_ratio", 
                state.interpolation_ratio, 
                "Value interpolation ratio"
            );
            step.metadata.set(
                "formula", 
                "pos = left + ((target - arr[left]) * (right - left)) / (arr[right] - arr[left])", 
                "Interpolation formula"
            );

            // Calculate expected position for uniformly distributed data
            double expected_ratio = static_cast<double>(state.target - state.left_value) /
                                   (state.right_value - state.left_value);
            step.metadata.set(
                "expected_ratio", 
                expected_ratio, 
                "Expected position ratio"
            );
            step.metadata.set(
                "guess_accuracy", 
                state.guess_accuracies.empty() ? 0.0 : state.guess_accuracies.back(), 
                "Guess accuracy"
            );
        }

        // Add current value at position if valid
        if (state.pos < n)
        {
            step.metadata.set(
                "pos_value", 
                state.data[state.pos], 
                "Value at estimated position"
            );
        }

        // Add interval size
        if (state.right >= state.left)
        {
            step.metadata.set(
                "interval_size", 
                state.right - state.left + 1, 
                "Current search interval size"
            );
        }

        // Context for specific operations
        if (operation_id == "narrow_left")
        {
            step.metadata.set(
                "new_right", 
                state.right, 
                "New right boundary after narrowing left"
            );
            step.metadata.set(
                "direction", 
                "left", 
                "Search direction"
            );
        }
        else if (operation_id == "narrow_right")
        {
            step.metadata.set(
                "new_left", 
                state.left, 
                "New left boundary after narrowing right"
            );
            step.metadata.set(
                "direction", 
                "right", 
                "Search direction"
            );
        }
        else if (operation_id == "found")
        {
            step.metadata.set(
                "found_status", 
                "found", 
                "Search result status"
            );
        }
        else if (operation_id == "not_found" || operation_id == "out_of_range")
        {
            step.metadata.set(
                "found_status", 
                "not found", 
                "Search result status"
            );
        }
        else if (operation_id == "completed")
        {
            std::string found_status = state.found ? "found" : "not found";
            step.metadata.set(
                "found_status", 
                found_status, 
                "Whether target was found"
            );
        }

        // Adding tags from step mapping
        auto tags = get_step_tags(operation_id);
        for (const auto& tag : tags)
        {
            step.metadata.add_tag(tag);
        }
    }

    void InterpolationSearch::update_visualization_data(
        AlgorithmStep& step,
        const State& state,
        const std::string& operation_id,
        const std::vector<size_t>& probed_indices,
        const std::vector<size_t>& eliminated_indices
    ) const
    {
        auto& viz = step.visualization;

        // Basic metrics
        viz.comparison_count = state.comparisons;
        viz.target_value = state.target;

        // Clear previous highlights
        viz.highlighted_index = std::nullopt;
        viz.compared_index = std::nullopt;
        viz.additional_highlights.clear();

        // Populate search-specific visualization data
        viz.search.target_value = state.target;
        viz.search.is_searching = !state.found && state.left <= state.right;
        viz.search.search_step = state.iterations;

        // Calculate search progress
        size_t total_elements = state.data.size();
        size_t remaining = (state.right >= state.left) ? (state.right - state.left + 1) : 0;
        viz.search.search_progress = total_elements > 0 ?
            1.0f - static_cast<float>(remaining) / total_elements : 0.0f;

        // Set boundaries for visualization
        viz.search.left_boundary = state.left;
        viz.search.right_boundary = state.right;

        // Set searched indices (probed positions)
        viz.search.searched_indices = probed_indices;

        // Set eliminated indices
        viz.search.eliminated_indices = eliminated_indices;

        // Special handling for eliminated_regions (backward compatibility)
        viz.eliminated_regions = eliminated_indices;

        // Set current focus based on phase
        if (state.phase == SearchPhase::CALCULATING_POSITION ||
            state.phase == SearchPhase::COMPARING)
        {
            if (state.pos < state.data.size())
            {
                viz.highlighted_index = state.pos;
                viz.compared_index = state.pos;
                viz.search.mid_point = state.pos;
            }
        }
        else
        {
            viz.search.mid_point = std::nullopt;
        }

        // Add interpolation visualization data to metadata for custom renderers
        step.metadata.set(
            "interpolation_ratio", 
            state.interpolation_ratio, 
            "Visual interpolation ratio"
        );
        step.metadata.set(
            "phase", 
            static_cast<int>(state.phase), 
            "Current search phase"
        );

        // Set found index if applicable
        if (state.found && state.result_index >= 0)
        {
            viz.search.found_index = static_cast<size_t>(state.result_index);
            viz.is_found = true;

            // Also add to additional highlights for compatibility
            viz.additional_highlights.push_back(static_cast<size_t>(state.result_index));
        }
        else
        {
            viz.search.found_index = std::nullopt;
            viz.is_found = false;
        }

        // Store guess accuracy for visualization
        if (!state.guess_accuracies.empty())
        {
            step.metadata.set(
                "guess_accuracy", 
                state.guess_accuracies.back(), 
                "Accuracy of last guess"
            );
        }

        // Add performance prediction
        step.metadata.set(
            "predicted_efficiency",
            m_data_uniformity > 0.7 ? "Excellent" : (m_data_uniformity > 0.4 ? "Good" : "Poor"),
            "Expected performance based on data distribution"
        );
    }

    size_t InterpolationSearch::calculate_position(
        const State& state
    ) const
    {
        if (state.left_value == state.right_value)
            return state.left;

        // Formula: pos = left + ((target - arr[left]) * (right - left)) / (arr[right] - arr[left])
        long long numerator = static_cast<long long>(state.target - state.left_value) *
                             (state.right - state.left);
        long long denominator = state.right_value - state.left_value;

        if (denominator == 0) return state.left;

        // Prevent overflow
        size_t pos = state.left + static_cast<size_t>(numerator / denominator);

        // Clamp to valid range
        if (pos < state.left) pos = state.left;
        if (pos > state.right) pos = state.right;

        return pos;
    }

    double InterpolationSearch::calculate_interpolation_ratio(
        const State& state
    ) const
    {
        if (state.left_value == state.right_value)
            return 0.5;

        return static_cast<double>(state.target - state.left_value) /
               static_cast<double>(state.right_value - state.left_value);
    }

    /**
     * @brief Calculate guess accuracy
     */
    double InterpolationSearch::calculate_guess_accuracy(
        const State& state
    ) const
    {
        if (state.left >= state.right) return 0.0;

        size_t actual_position = 0;
        for (size_t i = state.left; i <= state.right; ++i)
        {
            if (state.data[i] == state.target)
            {
                actual_position = i;
                break;
            }
        }

        if (actual_position < state.left || actual_position > state.right)
            return 0.0;

        double error = std::abs(
            static_cast<double>(state.pos) - static_cast<double>(actual_position)) /
                (state.right - state.left);
        return 1.0 - std::min(1.0, error);
    }

    double InterpolationSearch::calculate_uniformity(
        const std::vector<int>& data
    ) const
    {
        if (data.size() < 2) return 1.0;

        double expected_step = static_cast<double>(data.back() - data.front()) / 
            (data.size() - 1);
        double total_error = 0.0;

        for (size_t i = 1; i < data.size(); ++i)
        {
            double actual_step = data[i] - data[i-1];
            total_error += std::abs(actual_step - expected_step);
        }

        double avg_error = total_error / (data.size() - 1);
        return 1.0 - std::min(1.0, avg_error / expected_step);
    }

} // namespace c2l::algorithms