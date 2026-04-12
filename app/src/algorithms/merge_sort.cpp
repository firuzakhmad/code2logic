#include "algorithms/merge_sort.hpp"
#include "core/utils/logger/logger.hpp"

namespace c2l::algorithms
{
    MergeSort::MergeSort(core::JsonConfigManager& json_config_manager)
        : JsonAlgorithmBase(json_config_manager, AlgorithmType::MERGE_SORT)
    {
        LOG_DEBUG("MergeSort created and metadata loaded from JSON");
    }

    void MergeSort::initialize(const std::vector<int>& data)
    {
        m_original_data = data;
        reset();
        generate_all_steps();
        notify_observers();
        
        LOG_INFO(
            "MergeSort initialized with {} elements. Generated {} steps.",
            data.size(), 
            m_steps.size()
        );
    }

    bool MergeSort::step_forward()
    {
        if (m_current_step_index < m_steps.size() - 1)
        {
            m_current_step_index++;
            notify_observers();
            return true;
        }
        return false;
    }

    bool MergeSort::step_backward()
    {
        if (m_current_step_index > 0)
        {
            m_current_step_index--;
            notify_observers();
            return true;
        }
        return false;
    }

    void MergeSort::reset()
    {
        m_current_step_index = 0;
        m_steps.clear();
        m_total_comparisons = 0;
        m_total_copies = 0;
    }

    std::vector<int> MergeSort::get_original_data() const
    {
        return m_original_data;
    }

    AlgorithmStep MergeSort::get_current_step() const
    {
        if (is_steps_empty_or_invalid()) return {};
        return m_steps[m_current_step_index];
    }

    size_t MergeSort::get_step_count() const
    {
        return m_steps.size();
    }

    size_t MergeSort::get_current_step_index() const
    {
        return m_current_step_index;
    }

    bool MergeSort::is_complete() const
    {
        return !m_steps.empty() && m_current_step_index == m_steps.size() - 1;
    }

    bool MergeSort::is_steps_empty_or_invalid() const
    {
        return m_steps.empty() || m_current_step_index >= m_steps.size();
    }

    void MergeSort::generate_all_steps()
    {
        if (m_original_data.empty())
            return;

        m_steps.clear();
        m_total_comparisons = 0;
        m_total_copies = 0;

        MergeSortState state;
        state.data = m_original_data;
        state.aux.resize(m_original_data.size());

        push_step(state, "init");

        merge_sort_recursive(state, 0, state.data.size() - 1, 0);

        state.is_complete = true;
        push_step(state, "completed");

        LOG_DEBUG(
            "Generated {} steps for Merge Sort", 
            m_steps.size()
        );
    }

    void MergeSort::merge_sort_recursive(
        MergeSortState& state,
        size_t left,
        size_t right,
        size_t depth)
    {
        state.current_merge = MergeRange(left, left, right, depth);

        push_step(state, "check_base_case");
        if (left >= right)
        {
            push_step(state, "base_case_hit");
            return;
        }

        size_t mid = left + (right - left) / 2;

        state.current_merge = MergeRange(left, mid, right, depth);

        push_step(state, "divide");

        push_step(state, "recurse_left");
        merge_sort_recursive(state, left, mid, depth + 1);


        state.current_merge = MergeRange(left, mid, right, depth);
        push_step(state, "return_recursion_left");

        push_step(state, "recurse_right");
        merge_sort_recursive(state, mid + 1, right, depth + 1);

        state.current_merge = MergeRange(left, mid, right, depth);
        push_step(state, "return_recursion_right");

        merge(state, left, mid, right, depth);
    }

    void MergeSort::merge(
        MergeSortState& state,
        size_t left,
        size_t mid,
        size_t right,
        size_t depth)
    {
        state.current_merge = MergeRange(left, mid, right, depth);

        push_step(state, "merge_start");

        std::vector<int> left_array(mid - left + 1);
        std::vector<int> right_array(right - mid);

        for (size_t i = 0; i < left_array.size(); ++i)
            left_array[i] = state.data[left + i];

        for (size_t j = 0; j < right_array.size(); ++j)
            right_array[j] = state.data[mid + 1 + j];

        size_t i = 0, j = 0, k = left;

        push_step(state, "setup");

        while (i < left_array.size() && j < right_array.size())
        {
            state.merge_step.left_index  = left + i;
            state.merge_step.right_index = mid + 1 + j;
            state.merge_step.target_index = k;

            state.highlighted_left  = state.merge_step.left_index;
            state.highlighted_right = state.merge_step.right_index;

            push_step(state, "compare");

            ++m_total_comparisons;
            state.comparisons = m_total_comparisons;

            if (left_array[i] <= right_array[j])
            {
                state.data[k] = left_array[i];

                state.merge_step.left_taken = true;
                state.merge_step.right_taken = false;

                push_step(state, "take_left");
                ++i;
            }
            else
            {
                state.data[k] = right_array[j];

                state.merge_step.left_taken = false;
                state.merge_step.right_taken = true;

                push_step(state, "take_right");
                ++j;
            }

            m_total_copies++;
            state.copies = m_total_copies;

            ++k;
        }

        while (i < left_array.size())
        {
            state.merge_step.left_index = left + i;
            state.merge_step.target_index = k;
            state.merge_step.left_taken = true;
            state.merge_step.right_taken = false;

            state.data[k++] = left_array[i++];

            ++m_total_copies;
            state.copies = m_total_copies;

            push_step(state, "copy_left_remaining");
        }

        while (j < right_array.size())
        {
            state.merge_step.right_index  = mid + 1 + j;
            state.merge_step.target_index = k;
            state.merge_step.left_taken   = false;
            state.merge_step.right_taken  = true;

           state.data[k++] = right_array[j++];

            ++m_total_copies;
            state.copies = m_total_copies;

            push_step(state, "copy_right_remaining");
        }

        push_step(state, "merge_complete");
    }

    void MergeSort::push_step(
        const MergeSortState& state,
        const std::string& operation_id)
    {
        auto step = create_step_from_state(state, operation_id);
        m_steps.push_back(std::move(step));
    }

    void MergeSort::populate_step_metadata(
        AlgorithmStep& step,
        const MergeSortState& state,
        const std::string& operation_id
    ) const
    {
        step.metadata.set(
            "size", 
            state.data.size(), 
            "Array size"
        );
        step.metadata.set(
            "left", 
            state.current_merge.left, 
            "Left index"
        );
        step.metadata.set(
            "mid", 
            state.current_merge.mid, 
            "Middle index"
        );
        step.metadata.set(
            "mid+1", 
            state.current_merge.mid + 1, 
            "Middle index"
        );
        step.metadata.set(
            "right", 
            state.current_merge.right, 
            "Right index"
        );
        step.metadata.set(
            "depth", 
            state.current_merge.depth, 
            "Recursion depth"
        );

        // Only set merge-related fields when VALID
        if (operation_id == "compare" ||
            operation_id == "take_left" ||
            operation_id == "take_right" ||
            operation_id == "copy_left_remaining" ||
            operation_id == "copy_right_remaining")
        {
            step.metadata.set(
                "left_idx", 
                state.merge_step.left_index, 
                "Left pointer"
            );
            step.metadata.set(
                "right_idx", 
                state.merge_step.right_index, 
                "Right pointer"
            );
            step.metadata.set(
                "target_idx", 
                state.merge_step.target_index, 
                "Target index"
            );

            if (state.merge_step.left_index < state.data.size())
            {
                step.metadata.set(
                    "left_val",
                    state.data[state.merge_step.left_index], 
                    "Left value"
                );
            }

            if (state.merge_step.right_index < state.data.size())
            {
                step.metadata.set(
                    "right_val",
                    state.data[state.merge_step.right_index], 
                    "Right value"
                );
            }
        }

        step.metadata.set(
            "comparisons", 
            state.comparisons, 
            "Total comparisons"
        );
        step.metadata.set(
            "copies", 
            state.copies, 
            "Total copies"
        );

        size_t sub_size = (state.current_merge.right >= state.current_merge.left)
            ? (state.current_merge.right - state.current_merge.left + 1)
            : 0;

        step.metadata.set(
            "subarray_size", 
            sub_size, 
            "Subarray size"
        );

        for (const auto& tag : get_step_tags(operation_id))
        {
            step.metadata.add_tag(tag);
        }
    }

    AlgorithmStep MergeSort::create_step_from_state(
        const MergeSortState& state,
        const std::string& operation_id) const
    {
        AlgorithmStep step;

        step.data = state.data;
        step.metadata.operation_id = operation_id;

        // Populate metadata
        populate_step_metadata(step, state, operation_id);

        // Generate description using JSON template
        step.description = format_step_description(operation_id, step);

        // Update visualization
        update_visualization_data(step, state, operation_id);

        return step;
    }

    void MergeSort::update_visualization_data(
        AlgorithmStep& step,
        const MergeSortState& state,
        const std::string& operation_id) const
    {
        auto& viz = step.visualization;
        
        // Basic metrics
        viz.comparisons = state.comparisons;
        viz.swaps = 0;  // Merge sort doesn't use swaps
        viz.memory_usage = state.aux.size() * sizeof(int);
        
        // Clear previous highlights
        viz.highlighted_index = std::numeric_limits<size_t>::max();
        viz.compared_index = std::numeric_limits<size_t>::max();
        viz.additional_highlights.clear();
        
        // Set subarray boundaries
        if (state.current_merge.left < state.data.size())
        {
            viz.subarray_low = state.current_merge.left;
        }
        if (state.current_merge.right < state.data.size())
        {
            viz.subarray_high = state.current_merge.right;
        }
        
        // Set merge boundaries
        if (state.current_merge.mid < state.data.size())
        {
            viz.merge_boundary = state.current_merge.mid;
        }
        
        // Set highlights based on operation
        if (operation_id == "compare")
        {
            if (state.highlighted_left < state.data.size())
            {
                viz.highlighted_index = state.highlighted_left;
            }
            if (state.highlighted_right < state.data.size())
            {
                viz.compared_index = state.highlighted_right;
            }
            viz.additional_highlights.push_back(state.highlighted_left);
            viz.additional_highlights.push_back(state.highlighted_right);
        }
        else if (operation_id == "copy_left_remaining" ||
                 operation_id == "copy_right_remaining")
        {
            // Highlight the element being copied
            if (state.merge_step.left_taken && 
                state.merge_step.left_index > 0)
            {
                viz.highlighted_index = state.merge_step.left_index - 1;
            }
            else if (state.merge_step.right_taken && state.merge_step.right_index > 0)
            {
                viz.highlighted_index = state.merge_step.right_index - 1;
            }
            
            // Highlight the target position
            if (state.merge_step.target_index > 0)
            {
                viz.compared_index = state.merge_step.target_index - 1;
            }
        }
        else if (operation_id == "merge_start")
        {
            // Highlight the entire subarray being merged
            for (size_t i = state.current_merge.left; 
                 i <= state.current_merge.right && i < state.data.size(); ++i)
            {
                viz.additional_highlights.push_back(i);
            }
        }
        else if (operation_id == "merge_complete")
        {
            // Highlight the merged portion
            for (size_t i = state.current_merge.left; 
                 i <= state.current_merge.right && i < state.data.size(); ++i)
            {
                viz.additional_highlights.push_back(i);
            }
            viz.is_merge_complete = true;
        }

        // Mark recursion/merge depth
        viz.recursion_depth = state.current_merge.depth;
        
        // Special flags for merge sort
        viz.is_merge_step =
            (operation_id == "merge_start" ||
             operation_id == "compare" ||
             operation_id == "take_left" ||
             operation_id == "take_right" ||
             operation_id == "copy_left_remaining" ||
             operation_id == "copy_right_remaining");
        viz.is_compare_step = (operation_id == "compare");
    }

} // namespace c2l::algorithms