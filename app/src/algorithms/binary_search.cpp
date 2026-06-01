#include "binary_search.hpp"
#include "core/utils/logger/logger.hpp"
#include <algorithm>
#include <limits>

namespace c2l::algorithms
{
    BinarySearch::BinarySearch(core::JsonConfigManager& json_config_manager)
        : JsonAlgorithmBase(json_config_manager, AlgorithmType::BINARY_SEARCH)
    {
        LOG_DEBUG("BinarySearch created and metadata loaded from JSON");
    }

    void BinarySearch::set_search_target(
        const int target)
    {
        m_target = target;
        reset();
        generate_all_steps();
    }


    void BinarySearch::reset_state()
    {
        m_total_comparisons = 0;
        m_result_index = -1;
    }

    void BinarySearch::generate_all_steps()
    {
        if (m_original_data.empty())
        {
            LOG_WARNING("Cannot generate steps: empty data");
            return;
        }

        m_steps.clear();
        m_total_comparisons = 0;
        m_result_index = -1;

        // Initializing state
        State state;
        state.data = m_original_data;
        state.target = m_target;
        state.left = 0;
        state.right = m_original_data.size() - 1;
        state.comparisons = 0;
        state.result_index = -1;
        state.found = false;
        state.iteration = 0;

        // Track searched and eliminated indices
        std::vector<size_t> searched_indices;
        std::vector<size_t> eliminated_indices;

        // Initialization step
        push_step(
            state,
            "init",
            searched_indices,
            eliminated_indices
        );

        // Performing binary search
        perform_search(
            state,
            searched_indices,
            eliminated_indices
        );

        // Final completion step
        push_step(
            state,
            "completed",
            searched_indices,
            eliminated_indices
        );

        LOG_DEBUG(
            "Generated {} steps for binary search (target: {})",
            m_steps.size(),
            m_target
        );
    }

    void BinarySearch::perform_search(
        State& state,
        std::vector<size_t>& searched_indices,
        std::vector<size_t>& eliminated_indices)
    {
        size_t left = state.left;
        size_t right = state.right;
        size_t iteration = 0;

        while (left <= right && !state.found)
        {
            iteration++;
            state.iteration = iteration;
            state.left = left;
            state.right = right;

            // Loop start step
            push_step(
                state,
                "loop_start",
                searched_indices,
                eliminated_indices
            );

            // Calculating middle index (safe from overflow)
            size_t mid = calculate_mid(left, right);
            state.mid = mid;
            state.mid_value = state.data[mid];

            // Add current mid to searched indices
            searched_indices.push_back(mid);

            // Mid calculation step
            push_step(
                state,
                "mid_calculation",
                searched_indices,
                eliminated_indices
            );

            // Comparing for equality
            m_total_comparisons++;
            state.comparisons = m_total_comparisons;
            push_step(
                state,
                "compare_equal",
                searched_indices,
                eliminated_indices
            );

            if (state.data[mid] == state.target)
            {
                // Target found
                state.found = true;
                state.result_index = static_cast<int>(mid);
                m_found = true;
                m_result_index = static_cast<int>(mid);
                push_step(
                    state,
                    "found",
                    searched_indices,
                    eliminated_indices
                );
                break;
            }

            // Comparing for less than
            m_total_comparisons++;
            state.comparisons = m_total_comparisons;
            push_step(
                state,
                "compare_less",
                searched_indices,
                eliminated_indices
            );

            if (state.data[mid] < state.target)
            {
                // Search right half - eliminate left half including mid
                for (size_t i = left; i <= mid; ++i)
                {
                    if (std::find(
                        eliminated_indices.begin(),
                        eliminated_indices.end(), i) == eliminated_indices.end())
                    {
                        eliminated_indices.push_back(i);
                    }
                }
                left = mid + 1;
                push_step(
                    state,
                    "search_right",
                    searched_indices,
                    eliminated_indices
                );
            }
            else
            {
                // Search left half - eliminate right half including mid
                for (size_t i = mid; i <= right; ++i)
                {
                    if (std::find(
                        eliminated_indices.begin(),
                        eliminated_indices.end(), i) == eliminated_indices.end())
                    {
                        eliminated_indices.push_back(i);
                    }
                }
                right = mid - 1;
                push_step(
                    state,
                    "search_left",
                    searched_indices,
                    eliminated_indices
                );
            }
        }

        // If not found, set result to -1
        if (!state.found)
        {
            state.result_index = -1;
            m_result_index = -1;
            push_step(
                state,
                "not_found",
                searched_indices,
                eliminated_indices
            );
        }
    }

    void BinarySearch::push_step(
        const State& state,
        const std::string& operation_id,
        const std::vector<size_t>& searched_indices,
        const std::vector<size_t>& eliminated_indices)
    {
        auto step = create_step_from_state(
            state,
            operation_id,
            searched_indices,
            eliminated_indices
        );
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep BinarySearch::create_step_from_state(
        const State& state,
        const std::string& operation_id,
        const std::vector<size_t>& searched_indices,
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
            searched_indices,
            eliminated_indices
        );

        return step;
    }

    void BinarySearch::populate_step_metadata(
        AlgorithmStep& step, const State& state,
        const std::string& operation_id
    ) const
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
            "mid",
            state.mid,
            "Middle index of current interval"
        );
        step.metadata.set(
            "mid_value",
            state.mid_value,
            "Value at middle index"
        );
        step.metadata.set(
            "comparisons",
            state.comparisons,
            "Total comparisons performed"
        );
        step.metadata.set(
            "interval_size",
            interval_size(state.left, state.right),
            "Current search interval size"
        );
        step.metadata.set(
            "iteration",
            state.iteration,
            "Current iteration number"
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
            n, "Total array size"
        );

        // Add context for specific operations
        if (operation_id == "search_right")
        {
            size_t left_new = state.left + 1;
            step.metadata.set(
                "left_new",
                left_new,
                "New left boundary after narrowing"
            );
        }
        else if (operation_id == "search_left")
        {
            size_t right_new = (state.right > 0) ? state.right - 1 : 0;
            step.metadata.set(
                "right_new",
                right_new,
                "New right boundary after narrowing"
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
        else if (operation_id == "not_found")
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

    void BinarySearch::update_visualization_data(
        AlgorithmStep& step,
        const State& state,
        const std::string& operation_id,
        const std::vector<size_t>& searched_indices,
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

        // Set search boundaries
        viz.subarray_low = state.left;
        viz.subarray_high = state.right;

        // Populate search-specific visualization data
        viz.search.target_value = state.target;
        viz.search.searched_indices = searched_indices;
        viz.search.eliminated_indices = eliminated_indices;
        viz.search.is_searching = !state.found && (state.left <= state.right);
        viz.search.search_step = state.iteration;

        // Calculate search progress
        size_t total_elements = state.data.size();
        size_t remaining = (state.right >= state.left) ? (state.right - state.left + 1) : 0;
        viz.search.search_progress = total_elements > 0 ?
            1.0f - static_cast<float>(remaining) / total_elements : 0.0f;

        // Set boundaries for visualization
        viz.search.left_boundary = state.left;
        viz.search.right_boundary = state.right;

        // Set mid point for current step
        if (operation_id != "init" && operation_id != "completed" &&
            state.mid < state.data.size())
        {
            viz.search.mid_point = state.mid;

            // Also set highlighted_index for compatibility with existing renderers
            viz.highlighted_index = state.mid;
        }

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

        // For comparison steps, set compared_index
        if (operation_id == "compare_equal" || operation_id == "compare_less")
        {
            if (state.mid < state.data.size())
            {
                viz.compared_index = state.mid;
            }
        }

        // Mark eliminated regions for backward compatibility
        viz.eliminated_regions = eliminated_indices;
    }

} // namespace c2l::algorithms