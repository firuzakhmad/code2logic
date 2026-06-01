#include "bubble_sort.hpp"

#include "core/utils/logger/logger.hpp"

namespace c2l::algorithms
{
    BubbleSort::BubbleSort(core::JsonConfigManager& json_config_manager)
        : JsonAlgorithmBase{json_config_manager, AlgorithmType::BUBBLE_SORT}
    {
        LOG_DEBUG("BubbleSort created and metadata loaded from JSON");
    }

    void BubbleSort::reset_state()
    {
        m_total_comparisons = 0;
        m_total_swaps = 0;
    }

    void BubbleSort::generate_all_steps()
    {
        if (m_original_data.empty()) 
        {
            LOG_WARNING("Cannot generate steps: empty data");
            return;
        }

        m_steps.clear();

        std::vector<int> data{m_original_data};
        const size_t n = data.size();

        // Initializing state
        BubbleSortState state{};
        state.data = data;

        push_step(
            data,
            state,
            "init",
            m_total_comparisons,
            m_total_swaps
        );

        // Main bubble sort algorithm 
        for (size_t i = 0; i < n - 1; ++i)
        {
            state.outer_loop_index = i;
            state.inner_loop_index = 0;
            state.swapped_in_current_pass = false;

            // Outer loop step
            push_step(
                data,
                state,
                "outer_loop",
                m_total_comparisons,
                m_total_swaps
            );

            for (size_t j = 0; j < n - i - 1; ++j)
            {
                state.inner_loop_index = j;

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
                push_step(
                    data,
                    state,
                    "compare",
                    m_total_comparisons,
                    m_total_swaps
                );

                if (data[j] > data[j + 1])
                {
                    // Swap step
                    std::swap(data[j], data[j + 1]);
                    m_total_swaps++;
                    state.swaps = m_total_swaps;

                    state.swapped_in_current_pass = true;

                    push_step(
                        data,
                        state,
                        "swap",
                        m_total_comparisons,
                        m_total_swaps
                    );
                }
            }

            // Passing complete step
            state.data = data;
            state.sorted_elements = i + 1;

            push_step(
                data,
                state,
                "pass_complete",
                m_total_comparisons,
                m_total_swaps
            );

            if (!state.swapped_in_current_pass)
            {
                push_step(
                    data,
                    state,
                    "completed",
                    m_total_comparisons,
                    m_total_swaps
                );

                return;
            }
        }

        // Final sorted state
        state.outer_loop_index = n - 1;
        state.inner_loop_index = 0;
        state.sorted_elements = n;
        state.data = data;

        push_step(
            data,
            state,
            "completed",
            m_total_comparisons,
            m_total_swaps
        );

        LOG_DEBUG("Generated {} steps for bubble sort", m_steps.size());
    }

    void BubbleSort::push_step(
        const std::vector<int>& data,
        const BubbleSortState& state,
        const std::string& operation_id,
        size_t total_comparisons,
        size_t total_swaps)
    {
        auto step = create_step_from_state(
            data,
            state,
            operation_id,
            total_comparisons,
            total_swaps
        );
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep BubbleSort::create_step_from_state(
        const std::vector<int>& data,
        const BubbleSortState& state,
        const std::string& operation_id,
        size_t total_comparisons,
        size_t total_swaps) const
    {
        AlgorithmStep step;

        step.data = data;
        step.metadata.operation_id = operation_id;

        populate_step_metadata(
            step,
            state,
            operation_id,
            total_comparisons,
            total_swaps
        );

        // Generating description using JSON template
        step.description = format_step_description(operation_id, step);


        return step;
    }

    void BubbleSort::populate_step_metadata(
        AlgorithmStep& step,
        const BubbleSortState& state,
        const std::string& operation_id,
        size_t total_comparisons,
        size_t total_swaps) const
    {
        // Setting core data
        step.metadata.set(
            "i", 
            state.outer_loop_index, 
            "Outer loop index"
        );
        step.metadata.set(
            "j", 
            state.inner_loop_index, 
            "Inner loop index"
        );
        if (operation_id == "pass_complete")
        {
            step.metadata.set(

                "swapped",
                state.swapped_in_current_pass,
                "Swap occurred during pass"
            );
        }

        step.metadata.set(
            "swapped", 
            state.swapped_in_current_pass, 
            "Swap occurred"
        );
        step.metadata.set(
            "comparisons", 
            state.comparisons, 
            "Total comparisons"
        );
        step.metadata.set(
            "swaps", 
            state.swaps, 
            "Total swaps"
        );
        step.metadata.set("sorted_count", 
            state.sorted_elements, 
        "Sorted elements"
        );

        // Setting array values if indices are valid
        if (state.inner_loop_index < state.data.size())
            step.metadata.set(
                "arr[j]", 
                state.data[state.inner_loop_index],
                std::string("Value at index ") + std::to_string(state.inner_loop_index)
            );

        if (state.inner_loop_index + 1 < state.data.size())
        {
            step.metadata.set(
                "j+1",
                state.inner_loop_index + 1,
                "Index j+1"
            );
            step.metadata.set(
                "arr[j+1]", 
                state.data[state.inner_loop_index + 1],
                std::string("Value at index ") + std::to_string(state.inner_loop_index + 1)
            );
        }

        // Adding tags from step mapping
        auto tags = get_step_tags(operation_id);
        for (const auto& tag : tags)
        {
            step.metadata.add_tag(tag);
        }

        // Visualization
        step.visualization.highlighted_index = state.inner_loop_index;
        if (state.inner_loop_index + 1 < state.data.size() &&
            operation_id == "compare" ||
            operation_id == "swap")
            step.visualization.compared_index = state.inner_loop_index + 1;
        else
            step.visualization.compared_index = -1;

        step.visualization.is_swap_step = (operation_id == "swap");
        step.visualization.comparison_count = total_comparisons;
        step.visualization.swap_count = total_swaps;

        // Marking sorted elements
        step.visualization.additional_highlights.clear();
        if (state.sorted_elements > 0)
        {
            for (size_t k = state.data.size() - state.sorted_elements; 
                 k < state.data.size(); ++k)
            {
                step.visualization.additional_highlights.push_back(k);
            }
        }
    }

} // namespace c2l::algorithms