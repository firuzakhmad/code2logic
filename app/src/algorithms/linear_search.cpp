#include "linear_search.hpp"
#include "core/utils/logger/logger.hpp"
#include <algorithm>
#include <chrono>
#include <numeric>

namespace c2l::algorithms
{
    LinearSearch::LinearSearch(core::JsonConfigManager& json_config_manager)
        : JsonAlgorithmBase(json_config_manager, AlgorithmType::LINEAR_SEARCH)
    {
        LOG_DEBUG("LinearSearch created and metadata loaded from JSON");
    }

    void LinearSearch::set_search_target(const int target)
    {
        m_target = target;
        reset();
        generate_all_steps();
    }

    void LinearSearch::reset_state()
    {
        m_result_index = -1;
        m_metrics = PerformanceMetrics{};
    }

    void LinearSearch::generate_all_steps()
    {
        if (m_original_data.empty())
        {
            LOG_WARNING("Cannot generate steps: empty data");
            return;
        }

        m_steps.clear();
        m_metrics.total_comparisons = 0;
        m_metrics.elements_checked = 0;
        m_result_index = -1;

        // Initializing state
        State state;
        state.data = m_original_data;
        state.target = m_target;
        state.current_index = 0;
        state.comparisons = 0;
        state.elements_checked = 0;
        state.result_index = -1;
        state.found = false;
        state.iteration = 0;
        state.search_progress = 0.0f;

        // Tracking checked indices
        std::vector<size_t> checked_indices;

        // Initialization step
        push_step(state, "init", checked_indices);

        // Performing linear search
        perform_search(state, checked_indices);

        // Final completion step
        push_step(state, "completed", checked_indices);

        // Update metrics
        m_metrics.total_comparisons = state.comparisons;
        m_metrics.elements_checked = state.elements_checked;
        m_metrics.found = state.found;
        m_metrics.result_index = state.result_index;

        LOG_DEBUG(
            "Generated {} steps for linear search (target: {})",
            m_steps.size(),
            m_target
        );
    }

    void LinearSearch::perform_search(
        State& state, 
        std::vector<size_t>& checked_indices)
    {
        const size_t n = state.data.size();
        
        for (size_t i = 0; i < n && !state.found; ++i)
        {
            state.iteration++;
            state.current_index = i;

            // Loop start step
            push_step(state, "loop_start", checked_indices);
            
            // Adding current index to checked indices
            checked_indices.push_back(i);
            state.elements_checked = checked_indices.size();
            
            // Comparison step
            state.comparisons++;
            push_step(state, "compare", checked_indices);
            
            if (state.data[i] == state.target)
            {
                // Target found
                state.found = true;
                state.result_index = static_cast<int>(i);
                m_result_index = static_cast<int>(i);
                push_step(state, "found", checked_indices);
                break;
            }
            else
            {
                // Continue searching
                push_step(state, "continue_search", checked_indices);
            }
        }
        
        // If not found
        if (!state.found)
        {
            state.result_index = -1;
            m_result_index = -1;
            push_step(state, "not_found", checked_indices);
        }
    }

    void LinearSearch::push_step(
        const State& state,
        const std::string& operation_id,
        const std::vector<size_t>& checked_indices)
    {
        auto step = create_step_from_state(
            state, 
            operation_id, 
            checked_indices
        );
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep LinearSearch::create_step_from_state(
        const State& state,
        const std::string& operation_id,
        const std::vector<size_t>& checked_indices
    ) const
    {
        AlgorithmStep step;
        
        // Core data
        step.data = state.data;
        step.metadata.operation_id = operation_id;
        
        // Populating metadata using JSON-driven approach
        populate_step_metadata(step, state, operation_id);
        
        // Generating description using JSON template
        step.description = format_step_description(
            operation_id, 
            step
        );
        
        // Updating visualization data
        update_visualization_data(
            step, 
            state, 
            operation_id, 
            checked_indices
        );
        
        return step;
    }

    void LinearSearch::populate_step_metadata(
        AlgorithmStep& step, 
        const State& state,
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
            "i", 
            state.current_index, 
            "Current index being examined"
        );
        
        if (state.current_index < n)
        {
            step.metadata.set(
                "current_value", 
                state.data[state.current_index],
                "Value at current index"
            );
        }
        
        step.metadata.set(
            "comparisons", 
            state.comparisons, 
            "Total comparisons performed"
        );
        step.metadata.set(
            "elements_checked", 
            state.elements_checked, 
            "Elements checked so far"
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
            n, 
            "Total array size"
        );
        step.metadata.set(
            "search_progress", 
            state.search_progress, 
            "Search progress through array"
        );
        
        // Add context for specific operations
        if (operation_id == "compare")
        {
            step.metadata.set(
                "are_equal", 
                (state.current_index < n && state.data[state.current_index] == state.target),
                "Whether current element equals target"
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

    void LinearSearch::update_visualization_data(
        AlgorithmStep& step,
        const State& state,
        const std::string& operation_id,
        const std::vector<size_t>& checked_indices
    ) const
    {
        auto& viz = step.visualization;

        // Basic metrics
        viz.comparison_count = state.comparisons;
        viz.target_value = state.target;

        // Clearing previous highlights
        viz.highlighted_index = std::nullopt;
        viz.compared_index = std::nullopt;
        viz.additional_highlights.clear();

        // Populating search-specific visualization data
        viz.search.target_value = state.target;
        viz.search.searched_indices = checked_indices;
        viz.search.is_searching = !state.found && 
            (state.current_index < state.data.size());
        viz.search.search_step = state.iteration;
        viz.search.search_progress = state.search_progress;
        
        // Setting current index for visualization
        if (state.current_index < state.data.size())
        {
            viz.highlighted_index = state.current_index;
            
            // For comparison steps, also set compared_index
            if (operation_id == "compare")
            {
                viz.compared_index = state.current_index;
            }
        }
        
        // Setting search boundaries (for linear search, boundaries are implicit)
        viz.search.left_boundary = 0;
        viz.search.right_boundary = state.data.size() - 1;
        viz.search.mid_point = std::nullopt;  // Linear search doesn't have a mid point
        
        // Setting found index if applicable
        if (state.found && state.result_index >= 0)
        {
            viz.search.found_index = static_cast<size_t>(state.result_index);
            viz.is_found = true;
            
            // Also add to additional highlights for compatibility
            viz.additional_highlights.push_back(
                static_cast<size_t>(state.result_index)
            );
        }
        else
        {
            viz.search.found_index = std::nullopt;
            viz.is_found = false;
        }
        
        // Marking eliminated indices (for linear search, all unchecked indices are potential)
        // But we'll mark unchecked indices as "to be checked" rather than eliminated
        std::vector<size_t> remaining_indices;
        for (size_t i = 0; i < state.data.size(); ++i)
        {
            if (std::find(
                checked_indices.begin(), 
                checked_indices.end(), i) == checked_indices.end())
            {
                remaining_indices.push_back(i);
            }
        }
        
        // For linear search, we don't eliminate indices until the search is complete
        if (operation_id == "not_found" || operation_id == "completed")
        {
            viz.search.eliminated_indices = remaining_indices;
        }
        else
        {
            viz.search.eliminated_indices.clear();
        }
        
        // Clearing eliminated_regions for compatibility (linear search doesn't eliminate early)
        viz.eliminated_regions.clear();
    }

} // namespace c2l::algorithms