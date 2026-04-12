#include "selection_sort.hpp"
#include "core/utils/logger/logger.hpp"
#include <algorithm>
#include <unordered_map>

namespace c2l::algorithms
{
    SelectionSort::SelectionSort(core::JsonConfigManager& json_config_manager)
        : JsonAlgorithmBase(json_config_manager, AlgorithmType::SELECTION_SORT)
    {
        LOG_DEBUG("SelectionSort created and metadata loaded from JSON");
    }

    void SelectionSort::initialize(const std::vector<int>& data)
    {
        if (data.empty())
        {
            LOG_WARNING("SelectionSort initialized with empty data");
            m_original_data.clear();
            reset();
            return;
        }

        m_original_data = data;
        reset();
        generate_all_steps();

        LOG_INFO(
            "SelectionSort initialized with {} elements. Generated {} steps.",
            data.size(), 
            m_steps.size()
        );
    }

    bool SelectionSort::step_forward()
    {
        if (m_current_step_index < m_steps.size() - 1)
        {
            m_current_step_index++;
            notify_observers();
            return true;
        }

        LOG_DEBUG("Cannot step forward - already at last step");
        return false;
    }

    bool SelectionSort::step_backward()
    {
        if (m_current_step_index > 0)
        {
            m_current_step_index--;
            notify_observers();
            return true;
        }

        LOG_DEBUG("Cannot step backward - already at first step");
        return false;
    }

    void SelectionSort::reset()
    {
        m_current_step_index = 0;
        m_steps.clear();
        m_total_comparisons = 0;
        m_total_swaps = 0;
        LOG_DEBUG("SelectionSort reset");
    }

    std::vector<int> SelectionSort::get_original_data() const
    {
        return m_original_data;
    }

    AlgorithmStep SelectionSort::get_current_step() const
    {
        if (m_steps.empty() || m_current_step_index >= m_steps.size())
        {
            LOG_WARNING(
                "get_current_step called with invalid state - steps: {}, index: {}",
                m_steps.size(), 
                m_current_step_index
            );
            return AlgorithmStep{};
        }
        return m_steps[m_current_step_index];
    }

    size_t SelectionSort::get_step_count() const
    {
        return m_steps.size();
    }

    size_t SelectionSort::get_current_step_index() const
    {
        return m_current_step_index;
    }

    bool SelectionSort::is_complete() const
    {
        if (m_steps.empty())
        {
            return false;
        }
        return m_current_step_index >= m_steps.size() - 1;
    }

    bool SelectionSort::is_steps_empty_or_invalid() const
    {
        if (m_steps.empty() || m_current_step_index >= m_steps.size())
        {
            return true;
        }
        return false;
    }

    void SelectionSort::generate_all_steps()
    {
        if (m_original_data.empty())
        {
            LOG_WARNING("Cannot generate steps: empty data");
            return;
        }

        m_steps.clear();
        m_total_comparisons = 0;
        m_total_swaps = 0;

        // Creating working copy
        std::vector<int> data = m_original_data;
        const size_t n = data.size();

        // Initial state
        SelectionSortState state;
        state.data = data;
        state.boundary_idx = 0;
        state.scan_idx = 0;
        state.min_idx = 0;
        state.min_value = data.empty() ? 0 : data[0];
        state.sorted_count = 0;
        state.comparisons = 0;
        state.swaps = 0;
        state.is_scanning = false;
        state.found_new_min = false;
        state.last_compared_idx = 0;

        // Initialization step
        push_step(
            data, 
            state, 
            "init", 
            m_total_comparisons, 
            m_total_swaps
        );

        // Main selection sort algorithm
        for (size_t i = 0; i < n - 1; ++i)
        {
            // Updating boundary index
            state.boundary_idx = i;
            state.sorted_count = i;
            state.is_scanning = false;
            
            // Outer loop step
            push_step(
                data, 
                state, 
                "outer_loop", 
                m_total_comparisons, 
                m_total_swaps
            );

            // Find minimum in unsorted portion
            size_t min_idx = i;
            state.min_idx = min_idx;
            state.min_value = data[min_idx];
            
            // Min selected step
            push_step(
                data, 
                state, 
                "min_selected", 
                m_total_comparisons, 
                m_total_swaps
            );

            // Scan the unsorted portion
            state.is_scanning = true;
            
            for (size_t j = i + 1; j < n; ++j)
            {
                state.scan_idx = j;
                
                // Inner loop step
                push_step(
                    data, 
                    state, 
                    "inner_loop", 
                    m_total_comparisons, 
                    m_total_swaps
                );
                
                // Comparison step
                m_total_comparisons++;
                state.comparisons = m_total_comparisons;
                state.last_compared_idx = j;
                state.found_new_min = false;
                
                push_step(
                    data, 
                    state, 
                    "compare", 
                    m_total_comparisons, 
                    m_total_swaps
                );
                
                if (data[j] < data[min_idx])
                {
                    // Found new minimum
                    size_t old_min_idx = min_idx;
                    min_idx = j;
                    state.min_idx = min_idx;
                    state.min_value = data[min_idx];
                    state.found_new_min = true;
                    
                    // Update minimum step with context about the change
                    std::unordered_map<std::string, std::string> context;
                    context["old_min_idx"] = std::to_string(old_min_idx);
                    context["old_min_value"] = std::to_string(data[old_min_idx]);
                    
                    push_step(
                        data, 
                        state, 
                        "update_min", 
                        m_total_comparisons, 
                        m_total_swaps, 
                        context
                    );
                }
            }
            
            // Reset scanning flag
            state.is_scanning = false;
            
            // Swap if needed
            if (min_idx != i)
            {
                // Perform swap
                std::swap(data[i], data[min_idx]);
                m_total_swaps++;
                state.swaps = m_total_swaps;
                state.data = data;
                
                // Swap step
                push_step(
                    data, 
                    state, 
                    "swap", 
                    m_total_comparisons, 
                    m_total_swaps
                );
            }
            else
            {
                // No swap needed
                push_step(
                    data, 
                    state, 
                    "no_swap", 
                    m_total_comparisons, 
                    m_total_swaps
                );
            }
            
            // Update sorted count and state after pass
            state.sorted_count = i + 1;
            state.data = data;
            
            // Pass complete step
            push_step(
                data, 
                state, 
                "pass_complete", 
                m_total_comparisons, 
                m_total_swaps
            );
        }

        // Final completion state
        state.boundary_idx = n - 1;
        state.sorted_count = n;
        state.is_scanning = false;
        state.data = data;
        
        // Completed step
        push_step(
            data, 
            state, 
            "completed", 
            m_total_comparisons, 
            m_total_swaps
        );

        LOG_DEBUG(
            "Generated {} steps for selection sort", 
            m_steps.size()
        );
    }

    void SelectionSort::push_step(
        const std::vector<int>& data,
        const SelectionSortState& state,
        const std::string& operation_id,
        size_t total_comparisons,
        size_t total_swaps,
        const std::unordered_map<std::string, std::string>& additional_context)
    {
        auto step = create_step_from_state(
            data,
            state,
            operation_id,
            total_comparisons,
            total_swaps,
            additional_context
        );
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep SelectionSort::create_step_from_state(
        const std::vector<int>& data,
        const SelectionSortState& state,
        const std::string& operation_id,
        size_t total_comparisons,
        size_t total_swaps,
        const std::unordered_map<std::string, std::string>& additional_context) const
    {
        AlgorithmStep step;
        
        // Core data
        step.data = data;
        step.metadata.operation_id = operation_id;
        
        // Populate metadata using JSON-driven approach
        populate_step_metadata(
            step, 
            state, 
            operation_id, 
            additional_context
        );
        
        // Add metrics
        step.metadata.set(
            "comparisons", 
            total_comparisons, 
            "Total comparisons performed"
        );
        step.metadata.set(
            "swaps", 
            total_swaps, 
            "Total swaps performed"
        );
        
        // Generate description using JSON template
        step.description = format_step_description(operation_id, step);
        
        // Update visualization data
        update_visualization_data(step, state, operation_id);
        
        return step;
    }

    void SelectionSort::populate_step_metadata(
        AlgorithmStep& step,
        const SelectionSortState& state,
        const std::string& operation_id,
        const std::unordered_map<std::string, std::string>& additional_context
    ) const
    {
        const size_t n = state.data.size();
        
        // Core algorithm variables
        step.metadata.set(
            "size", 
            state.data.size(), 
            "Array size"
        );
        step.metadata.set(
            "i", 
            state.boundary_idx, 
            "Current boundary index (sorted/unsorted boundary)"
        );
        step.metadata.set(
            "j", 
            state.scan_idx, 
            "Current scanning index"
        );
        step.metadata.set(
            "min_idx", 
            state.min_idx, 
            "Index of current minimum element"
        );
        step.metadata.set(
            "min_value", 
            state.min_value, 
            "Current minimum value"
        );
        step.metadata.set(
            "sorted_count", 
            state.sorted_count, 
            "Number of elements already sorted"
        );
        step.metadata.set(
            "n", 
            n, 
            "Array size"
        );
        
        // Array values with bounds checking
        if (state.boundary_idx < n)
        {
            step.metadata.set(
                "arr[i]", 
                state.data[state.boundary_idx],
                std::string("Value at boundary index ") + std::to_string(state.boundary_idx)
            );
        }
        
        if (state.scan_idx < n)
        {
            step.metadata.set(
                "arr[j]", 
                state.data[state.scan_idx],
                std::string("Value at scan index ") + std::to_string(state.scan_idx)
            );
        }
        
        if (state.min_idx < n)
        {
            step.metadata.set(
                "arr[min_idx]", 
                state.data[state.min_idx],
                std::string("Value at minimum index ") + std::to_string(state.min_idx)
            );
        }
        
        // Adding context for update_min operation
        if (operation_id == "update_min" && 
            additional_context.count("old_min_idx"))
        {
            size_t old_min_idx = std::stoul(additional_context.at("old_min_idx"));
            step.metadata.set(
                "old_min_idx", 
                old_min_idx, 
                "Previous minimum index"
            );
            
            if (additional_context.count("old_min_value"))
            {
                step.metadata.set(
                    "arr[old_min_idx]", 
                    std::stoi(additional_context.at("old_min_value")),
                    "Previous minimum value"
                );
            }
        }
        
        // Adding visualization flags
        step.metadata.set(
            "is_scanning", 
            state.is_scanning, 
            "Whether currently in scanning phase"
        );
        step.metadata.set(
            "found_new_min", 
            state.found_new_min, 
            "Whether a new minimum was just found"
        );
        
        // Adding tags from step mapping
        auto tags = get_step_tags(operation_id);
        for (const auto& tag : tags)
        {
            step.metadata.add_tag(tag);
        }
        
        // Add step-specific metadata for better visualization
        if (operation_id == "outer_loop")
        {
            step.metadata.set(
                "unsorted_remaining", 
                n - state.boundary_idx, 
                "Number of unsorted elements remaining"
            );
        }
        else if (operation_id == "compare")
        {
            step.metadata.set(
                "comparison_result", 
                state.data[state.scan_idx] < state.data[state.min_idx],
                "Result of comparison (true if current element is smaller)"
            );
        }
        else if (operation_id == "swap")
        {
            step.metadata.set(
                "swapped_value_at_boundary", 
                state.data[state.boundary_idx],
                "Value placed at boundary position after swap"
            );
            step.metadata.set(
                "swapped_value_from_min", 
                state.min_value,
                "Minimum value that was swapped to boundary"
            );
        }
        else if (operation_id == "pass_complete")
        {
            step.metadata.set(
                "elements_sorted_this_pass", 
                1, 
                "Number of elements placed in correct position this pass"
            );
            step.metadata.set(
                "final_value_at_boundary", 
                state.data[state.boundary_idx],
                "Value now in its final sorted position"
            );
        }
    }

    void SelectionSort::update_visualization_data(
        AlgorithmStep& step,
        const SelectionSortState& state,
        const std::string& operation_id) const
    {
        auto& viz = step.visualization;
        
        // Basic metrics
        viz.comparisons = state.comparisons;
        viz.swaps = state.swaps;
        
        // Clear previous highlights
        viz.highlighted_index = std::numeric_limits<size_t>::max();
        viz.compared_index = std::numeric_limits<size_t>::max();
        viz.additional_highlights.clear();
        
        // Setting visualization based on operation type
        if (operation_id == "init")
        {
            // No specific highlights for initialization
            viz.subarray_low = 0;
            viz.subarray_high = state.data.size() - 1;
        }
        else if (operation_id == "outer_loop")
        {
            // Highlight the boundary position
            if (state.boundary_idx < state.data.size())
            {
                viz.highlighted_index = state.boundary_idx;
                viz.subarray_low = state.boundary_idx;
                viz.subarray_high = state.data.size() - 1;
            }
        }
        else if (operation_id == "min_selected")
        {
            // Highlight the current minimum candidate
            if (state.min_idx < state.data.size())
            {
                viz.highlighted_index = state.min_idx;
                viz.additional_highlights.push_back(state.min_idx);
            }
            // Mark the unsorted region
            if (state.boundary_idx < state.data.size())
            {
                viz.subarray_low = state.boundary_idx;
                viz.subarray_high = state.data.size() - 1;
            }
        }
        else if (operation_id == "inner_loop" || 
                 operation_id == "compare")
        {
            // Highlight both the current scan position and current minimum
            if (state.scan_idx < state.data.size())
            {
                viz.highlighted_index = state.scan_idx;
            }
            if (state.min_idx < state.data.size() && 
                state.min_idx != state.scan_idx)
            {
                viz.compared_index = state.min_idx;
                viz.additional_highlights.push_back(state.min_idx);
            }
            // Mark the unsorted region
            if (state.boundary_idx < state.data.size())
            {
                viz.subarray_low = state.boundary_idx;
                viz.subarray_high = state.data.size() - 1;
            }
            viz.is_swap_step = false;
        }
        else if (operation_id == "update_min")
        {
            // Highlight the new minimum found
            if (state.min_idx < state.data.size())
            {
                viz.highlighted_index = state.min_idx;
                viz.additional_highlights.push_back(state.min_idx);
            }
            // Also show the previous scan position
            if (state.scan_idx < state.data.size())
            {
                viz.compared_index = state.scan_idx;
            }
            viz.subarray_low = state.boundary_idx;
            viz.subarray_high = state.data.size() - 1;
        }
        else if (operation_id == "swap")
        {
            // Highlight both elements being swapped
            if (state.boundary_idx < state.data.size())
            {
                viz.highlighted_index = state.boundary_idx;
                viz.additional_highlights.push_back(state.boundary_idx);
            }
            if (state.min_idx < state.data.size() && 
                state.min_idx != state.boundary_idx)
            {
                viz.compared_index = state.min_idx;
                viz.additional_highlights.push_back(state.min_idx);
            }
            viz.is_swap_step = true;
        }
        else if (operation_id == "no_swap")
        {
            // Highlight that the boundary element is already correct
            if (state.boundary_idx < state.data.size())
            {
                viz.highlighted_index = state.boundary_idx;
                viz.additional_highlights.push_back(state.boundary_idx);
            }
            viz.is_swap_step = false;
        }
        else if (operation_id == "pass_complete")
        {
            // Mark the newly sorted element
            if (state.boundary_idx < state.data.size())
            {
                viz.highlighted_index = state.boundary_idx;
                viz.additional_highlights.push_back(state.boundary_idx);
            }
            // Mark all sorted elements
            for (size_t k = 0; k < state.sorted_count && k < state.data.size(); ++k)
            {
                viz.additional_highlights.push_back(k);
            }
        }
        else if (operation_id == "completed")
        {
            // Mark the entire array as sorted
            for (size_t k = 0; k < state.data.size(); ++k)
            {
                viz.additional_highlights.push_back(k);
            }
            viz.subarray_low = 0;
            viz.subarray_high = state.data.size() - 1;
        }
        
        // Mark sorted region for all steps after first pass
        if (state.sorted_count > 0 && 
            operation_id != "init")
        {
            for (size_t k = 0; k < state.sorted_count && k < state.data.size(); ++k)
            {
                // Avoid duplicate highlights
                if (std::find(viz.additional_highlights.begin(), 
                              viz.additional_highlights.end(), k) == viz.additional_highlights.end())
                {
                    viz.additional_highlights.push_back(k);
                }
            }
        }
    }

    void SelectionSort::add_comparison_step(
        const std::vector<int>& data,
        SelectionSortState& state,
        size_t j,
        size_t min_idx)
    {
        state.scan_idx = j;
        state.min_idx = min_idx;
        state.last_compared_idx = j;
        
        m_total_comparisons++;
        state.comparisons = m_total_comparisons;
        state.found_new_min = false;
        
        push_step(data, state, "compare", m_total_comparisons, m_total_swaps);
    }

} // namespace c2l::algorithms