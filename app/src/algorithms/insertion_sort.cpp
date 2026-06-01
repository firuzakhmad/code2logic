#include "algorithms/insertion_sort.hpp"

#include "core/utils/logger/logger.hpp"

namespace c2l::algorithms
{
    InsertionSort::InsertionSort(core::JsonConfigManager& json_config_manager)
        : JsonAlgorithmBase{json_config_manager, AlgorithmType::INSERTION_SORT}
    {
        LOG_DEBUG("InsertionSort created and metadata loaded from JSON");
    }

    void InsertionSort::generate_all_steps()
    {
        if (m_original_data.empty())
        {
            LOG_WARNING("InsertionSort: empty input");
            return;
        }

        m_steps.clear();
        m_total_comparisons = 0;
        m_total_swaps = 0;

        m_steps.reserve(m_original_data.size() * m_original_data.size());

        // Initialize state
        InsertionSortState state{};
        state.data = m_original_data;
        state.sorted_elements = 1;

        // Initial step
        push_step(state, "init");

        const size_t n = state.data.size();

        // If array has only one element, we're done
        if (n <= 1)
        {
            push_step(state, "completed");
            LOG_DEBUG(
                "Generated {} steps for insertion sort (single element)",
                m_steps.size()
            );
            return;
        }

        // Main insertion sort algorithm
        for (size_t i = 1; i < n; ++i)
        {
            state.outer_loop_index = i;
            state.current_key = state.data[i];
            state.shifting_active = false;
            
            // Outer loop step - picking next element to insert
            push_step(state, "select_element");

            // Inserting the current element into the sorted portion
            size_t j = i;
            
            // Comparing and shift
            while (j > 0 && state.data[j - 1] > state.current_key)
            {
                state.inner_loop_index = j;

                // Comparison step
                ++m_total_comparisons;
                state.comparisons = m_total_comparisons;
                push_step(state, "compare");

                if (state.data[j - 1] <= state.current_key)
                    break;

                state.shifting_active = true;

                // Shifting element to the right
                state.data[j] = state.data[j - 1];
                ++m_total_swaps;
                state.swaps = m_total_swaps;
                
                push_step(state, "shift");
                
                --j;
            }
            
            // Inserting the key at its correct position
            state.data[j] = state.current_key;

            ++m_total_swaps;
            state.swaps = m_total_swaps;

            state.sorted_elements = i + 1;
            state.inner_loop_index = j;
            state.shifting_active = false;

            push_step(state, "insert");
        }

        push_step(state, "completed");

        LOG_DEBUG(
            "Generated {} steps for insertion sort", 
            m_steps.size()
        );
    }

    void InsertionSort::push_step(
        const InsertionSortState& state,
        const std::string& operation_id)
    {
        auto step = create_step_from_state(
            state,
            operation_id
        );
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep InsertionSort::create_step_from_state(
        const InsertionSortState& state,
        const std::string& operation_id) const
    {
        AlgorithmStep step;

        step.data = state.data;
        step.metadata.operation_id = operation_id;

        populate_step_metadata(
            step,
            state,
            operation_id
        );

        // Generating description using JSON template
        step.description = format_step_description(operation_id, step);


        return step;
    }

    void InsertionSort::populate_step_metadata(
        AlgorithmStep& step,
        const InsertionSortState& state,
        const std::string& operation_id) const
    {
        // Core
        step.metadata.set(
            "size", 
            state.data.size(), 
            "Array size"
        );
        step.metadata.set(
            "i", 
            state.outer_loop_index, 
            "Current index"
        );
        step.metadata.set(
            "j", 
            state.inner_loop_index, 
            "Comparison index"
        );
        step.metadata.set(
            "key", 
            state.current_key, 
            "Key value"
        );
        step.metadata.set(
            "shifting", 
            state.shifting_active, 
            "Shifting"
        );

        // Metrics
        step.metadata.set(
            "comparisons", 
            state.comparisons, 
            "Comparisons"
        );
        step.metadata.set(
            "swaps", 
            state.swaps, 
            "Assignments"
        );
        step.metadata.set(
            "sorted_count", 
            state.sorted_elements, 
            "Sorted elements"
        );

        if (state.outer_loop_index < state.data.size())
        {
            step.metadata.set(
                "arr[i]", 
                state.data[state.outer_loop_index], 
                "arr[i]"
            );
        }

        if (state.inner_loop_index < state.data.size())
        {
            step.metadata.set(
                "arr[j]", 
                state.data[state.inner_loop_index], 
                "arr[j]"
            );
        }

        if (state.inner_loop_index > 0)
        {
            step.metadata.set(
                "arr[j-1]",
                state.data[state.inner_loop_index - 1],
                "arr[j-1]"
            );
        }
        else
        {
            step.metadata.set("arr[j-1]", 0, "boundary");
        }

        // Tags
        for (const auto& tag : get_step_tags(operation_id))
            step.metadata.add_tag(tag);


        // Visualization part
        auto& viz = step.visualization;

        viz.is_shift_step = false;
        viz.is_insertion_step = false;
        viz.is_swap_step = false;
        viz.additional_highlights.clear();

        viz.highlighted_index = std::numeric_limits<size_t>::max();
        viz.compared_index = std::numeric_limits<size_t>::max();

        if (operation_id == "select_element")
        {
            viz.highlighted_index = state.outer_loop_index;
        }
        else if (operation_id == "compare")
        {
            if (state.inner_loop_index > 0)
            {
                viz.highlighted_index = state.inner_loop_index;
                viz.compared_index = state.inner_loop_index - 1;
            }
        }
        else if (operation_id == "shift")
        {
            viz.highlighted_index = state.inner_loop_index;
            viz.compared_index = state.inner_loop_index - 1;
            viz.is_shift_step = true;
        }
        else if (operation_id == "insert")
        {
            viz.highlighted_index = state.inner_loop_index;
            viz.is_insertion_step = true;
        }

        // Sorted region highlight
        for (size_t i = 0; i < state.sorted_elements && i < state.data.size(); ++i)
        {
            viz.additional_highlights.push_back(i);
        }

        viz.comparison_count = state.comparisons;
        viz.swap_count = state.swaps;
    }

} // namespace c2l::algorithms