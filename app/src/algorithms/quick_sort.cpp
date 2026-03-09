#include "algorithms/quick_sort.hpp"
#include "core/utils/logger/logger.hpp"

namespace c2l::algorithms
{
    QuickSort::QuickSort(core::JsonConfigManager& json_config_manager)
        : JsonAlgorithmBase(json_config_manager, AlgorithmType::QUICK_SORT)
    {
        LOG_DEBUG("QuickSort created and metadata loaded from JSON");
    }

    void QuickSort::initialize(const std::vector<int>& data)
    {
        m_original_data = data;
        reset();
        generate_all_steps();
        
        LOG_INFO("QuickSort initialized with {} elements. Generated {} steps.",
                data.size(), m_steps.size());
    }

    bool QuickSort::step_forward()
    {
        if (m_current_step_index < m_steps.size() - 1)
        {
            m_current_step_index++;
            notify_observers();
            return true;
        }
        return false;
    }

    bool QuickSort::step_backward()
    {
        if (m_current_step_index > 0)
        {
            m_current_step_index--;
            notify_observers();
            return true;
        }
        return false;
    }

    void QuickSort::reset()
    {
        m_current_step_index = 0;
        m_steps.clear();
        m_total_comparisons = 0;
        m_total_swaps = 0;
    }

    std::vector<int> QuickSort::get_original_data() const
    {
        return m_original_data;
    }

    AlgorithmStep QuickSort::get_current_step() const
    {
        if (m_steps.empty() || m_current_step_index >= m_steps.size())
        {
            return AlgorithmStep{};
        }
        return m_steps[m_current_step_index];
    }

    size_t QuickSort::get_step_count() const
    {
        return m_steps.size();
    }

    size_t QuickSort::get_current_step_index() const
    {
        return m_current_step_index;
    }

    bool QuickSort::is_complete() const
    {
        return m_current_step_index >= m_steps.size() - 1;
    }

    bool QuickSort::is_steps_empty_or_invalid() const
    {
        return m_steps.empty() || m_current_step_index >= m_steps.size();
    }

    void QuickSort::generate_all_steps()
    {
        if (m_original_data.empty())
        {
            LOG_WARNING("Cannot generate steps: empty data");
            return;
        }

        m_steps.clear();
        m_total_comparisons = 0;
        m_total_swaps = 0;
        
        // Initialize state
        QuickSortState state;
        state.data = m_original_data;
        state.comparisons = 0;
        state.swaps = 0;
        state.max_depth = 0;
        state.i = 0;
        state.j = 0;
        state.pivot = 0;
        state.pivot_index = 0;
        state.is_partitioning = false;

        // Initial step - pseudocode line 1
        push_step(state, "init");

        // Only sort if more than 1 element
        if (state.data.size() > 1)
        {
            Subarray initial{0, state.data.size() - 1, 0};
            state.stack.push(initial);

            // Process stack iteratively
            while (!state.stack.empty())
            {
                Subarray current = state.stack.top();
                state.stack.pop();
                
                // Validate subarray bounds
                if (current.low >= current.high || current.high >= state.data.size())
                {
                    continue;
                }
                
                state.current_subarray = current;
                state.max_depth = std::max(state.max_depth, current.depth);
                
                // Check if low < high
                if (current.low < current.high)
                {
                    // Partition start - preparing for partition
                    push_step(state, "partition_start");
                    
                    // Store partition boundaries for later steps
                    state.partition_low = current.low;
                    state.partition_high = current.high;
                    
                    // Perform partition and get pivot index
                    auto result = lomuto_partition(state, current, 
                                                   m_total_comparisons, 
                                                   m_total_swaps);
                    
                    
                    state.pivot_index = result.pivot_index;
                    state.comparisons = m_total_comparisons;
                    state.swaps = m_total_swaps;

                    push_step(state, "partition_complete");
                    

                    // Push right subarray first (so left is processed next)
                    // quick_sort(arr, pi + 1, high)
                    if (result.pivot_index + 1 < current.high)
                    {
                        Subarray right{result.pivot_index + 1, current.high, current.depth + 1};
                        state.stack.push(right);
                        push_step(state, "recursive_call_right");
                    }

                    // quick_sort(arr, low, pi - 1) 
                    if (current.low < result.pivot_index && result.pivot_index > 0)
                    {
                        Subarray left{current.low, result.pivot_index - 1, current.depth + 1};
                        state.stack.push(left);
                        push_step(state, "recursive_call");
                    }
                }
            }
        }

        // Final step
        push_step(state, "completed");

        LOG_DEBUG("Generated {} steps for Quick Sort", m_steps.size());
    }

    QuickSort::PartitionResult QuickSort::lomuto_partition(
        QuickSortState& state,
        Subarray& subarray,
        size_t& total_comparisons,
        size_t& total_swaps)
    {
        const size_t low = subarray.low;
        const size_t high = subarray.high;

        state.pivot = state.data[high];
        push_step(state, "pivot_selected");

        state.i = static_cast<int>(low) - 1;
        if (low == 0) 
        {
            state.i = static_cast<size_t>(-1);
        } else 
        {
            state.i = low - 1;
        }

        for (size_t j = low; j < high; ++j)
        {
            state.j = j;

            total_comparisons++;
            state.comparisons = total_comparisons;

            push_step(state, "partition_scan");

            if (state.data[j] <= state.pivot)
            {
                // Handling the case where i is sentinel (-1)
                if (state.i == static_cast<size_t>(-1)) 
                {
                    state.i = 0;
                } else 
                {
                    state.i++;
                }

                std::swap(state.data[state.i], state.data[j]);

                total_swaps++;
                state.swaps = total_swaps;

                push_step(state, "partition_swap");
            }
        }

            // Handling pivot position calculation safely
            size_t pivot_pos;
            if (state.i == static_cast<size_t>(-1)) 
            {
                pivot_pos = 0;
            } else 
            {
                pivot_pos = state.i + 1;
            }

        std::swap(state.data[pivot_pos], state.data[high]);

        total_swaps++;
        state.swaps = total_swaps;

        return PartitionResult{pivot_pos, total_comparisons, total_swaps};
    }

    void QuickSort::push_step(
        const QuickSortState& state,
        const std::string& operation_id)
    {
        auto step = create_step_from_state(state, operation_id);
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep QuickSort::create_step_from_state(
        const QuickSortState& state,
        const std::string& operation_id) const
    {
        AlgorithmStep step;
        
        // Core data
        step.data = state.data;
         
        // Populating metadata
        populate_step_metadata(step, state, operation_id);
        
        // Generating description using JSON template
        step.description = format_step_description(operation_id, step);
        
        // Set operation type for backward compatibility
        if (operation_id == "init") 
            step.metadata.operation_type = AlgorithmStepOperation::INIT;
        else if (operation_id == "partition_start") 
            step.metadata.operation_type = AlgorithmStepOperation::PARTITION_START;
        else if (operation_id == "pivot_selected") 
            step.metadata.operation_type = AlgorithmStepOperation::PIVOT_SELECTED;
        else if (operation_id == "partition_scan") 
            step.metadata.operation_type = AlgorithmStepOperation::PARTITION_SCAN;
        else if (operation_id == "partition_swap") 
            step.metadata.operation_type = AlgorithmStepOperation::PARTITION_SWAP;
        else if (operation_id == "partition_complete") 
            step.metadata.operation_type = AlgorithmStepOperation::PARTITION_COMPLETE;
        else if (operation_id == "recursive_call") 
            step.metadata.operation_type = AlgorithmStepOperation::RECURSIVE_CALL;
        else if (operation_id == "recursive_call_right") 
            step.metadata.operation_type = AlgorithmStepOperation::RECURSIVE_CALL_RIGHT;
        else if (operation_id == "completed") 
            step.metadata.operation_type = AlgorithmStepOperation::COMPLETED;
        else 
            step.metadata.operation_type = AlgorithmStepOperation::NONE;

        // Update visualization
        update_visualization_data(step, state, operation_id);

        return step;
    }

    void QuickSort::populate_step_metadata(
        AlgorithmStep& step,
        const QuickSortState& state,
        const std::string& operation_id) const
    {
        // Core indices
        step.metadata.set("low", state.current_subarray.low, "Low boundary");
        step.metadata.set("high", state.current_subarray.high, "High boundary");
        step.metadata.set("depth", state.current_subarray.depth, "Recursion depth");
        step.metadata.set("size", state.data.size(), "Array size");
        // Partition variables
        if (state.i != static_cast<size_t>(-1))
        {
            step.metadata.set("i", static_cast<int>(state.i), "Partition index");
            step.metadata.set("i+1", static_cast<int>(state.i + 1), "i + 1");
        }
        else
        {
            step.metadata.set("i", -1, "Partition index (initial)");
            step.metadata.set("i+1", 0, "i + 1");
        }
        
        step.metadata.set("j", state.j, "Scan index");
        step.metadata.set("pivot", state.pivot, "Pivot value");
        step.metadata.set("pivot_index", state.pivot_index, "Final pivot position");
        
        // For description templates that need i+1
        step.metadata.set("i+1", state.i + 1, "i + 1");
        
        // Metrics
        step.metadata.set("comparisons", state.comparisons, "Total comparisons");
        step.metadata.set("swaps", state.swaps, "Total swaps");
        step.metadata.set("max_depth", state.max_depth, "Maximum recursion depth");
        
        // Subarray size
        size_t subarray_size = 0;
        if (state.current_subarray.high >= state.current_subarray.low &&
            state.current_subarray.high < state.data.size())
        {
            subarray_size = state.current_subarray.high - state.current_subarray.low + 1;
        }
        step.metadata.set("size", subarray_size, "Subarray size");

        // Array values with bounds checking
        if (state.j < state.data.size())
        {
            step.metadata.set("arr[j]", state.data[state.j], 
                            std::string("Value at index ") + std::to_string(state.j));
        }
        
        if (state.i != static_cast<size_t>(-1) && state.i < state.data.size())
        {
            step.metadata.set("arr[i]", state.data[state.i],
                            std::string("Value at index ") + std::to_string(state.i));
        }
        
        if (state.pivot_index < state.data.size())
        {
            step.metadata.set("arr[pivot]", state.data[state.pivot_index],
                            std::string("Value at pivot index"));
        }

        // Special handling for partition_complete
        if (operation_id == "partition_complete")
        {
            size_t i_plus_1 = (state.i == static_cast<size_t>(-1)) ? 0 : state.i + 1;
            if (i_plus_1 < state.data.size())
            {
                step.metadata.set(
                    "arr[i+1]", 
                    state.data[i_plus_1],
                    std::string("Value at index i+1 (") + std::to_string(i_plus_1) + ")"
                );
            }
            if (state.current_subarray.high < state.data.size())
            {
                step.metadata.set(
                    "arr[high]", 
                    state.data[state.current_subarray.high],
                    std::string("Value at high index ") + std::to_string(state.current_subarray.high));
            }
        }

        // Adding tags from step mapping
        auto tags = get_step_tags(operation_id);
        for (const auto& tag : tags)
        {
            step.metadata.add_tag(tag);
        }
    }

    void QuickSort::update_visualization_data(
        AlgorithmStep& step,
        const QuickSortState& state,
        const std::string& operation_id) const
    {
        auto& viz = step.visualization;
        
        // Basic metrics
        viz.comparisons = state.comparisons;
        viz.swaps = state.swaps;
        
        // Clearing any previous highlights
        viz.highlighted_index = std::numeric_limits<size_t>::max();
        viz.compared_index = std::numeric_limits<size_t>::max();
        viz.additional_highlights.clear();
        
        // Setting highlighted indices based on operation
        if (operation_id == "pivot_selected")
        {
            if (state.current_subarray.high < state.data.size())
            {
                viz.highlighted_index = state.current_subarray.high;
                viz.additional_highlights.push_back(state.current_subarray.high);
            }
        }
        else if (operation_id == "partition_scan")
        {
            if (state.j < state.data.size())
            {
                viz.highlighted_index = state.j;
            }
            if (state.current_subarray.high < state.data.size())
            {
                viz.compared_index = state.current_subarray.high;
            }
            viz.is_swap_step = false;
        }
        else if (operation_id == "partition_swap")
        {
            if (state.i != static_cast<size_t>(-1) && state.i < state.data.size())
            {
                viz.highlighted_index = state.i;
                viz.additional_highlights.push_back(state.i);
            }
            if (state.j < state.data.size())
            {
                viz.compared_index = state.j;
                viz.additional_highlights.push_back(state.j);
            }
            viz.is_swap_step = true;
        }
        else if (operation_id == "partition_complete")
        {
            if (state.pivot_index < state.data.size())
            {
                viz.highlighted_index = state.pivot_index;
                viz.additional_highlights.push_back(state.pivot_index);
            }
            viz.is_partition_step = true;
        }
        else if (operation_id == "recursive_call" || 
                 operation_id == "recursive_call_right")
        {
            // Highlighting the subarray being processed
            if (state.current_subarray.low < state.data.size())
            {
                viz.subarray_low = state.current_subarray.low;
            }
            if (state.current_subarray.high < state.data.size())
            {
                viz.subarray_high = state.current_subarray.high;
            }
        }
        
        // Marking subarray boundaries
        if (state.current_subarray.low < state.data.size())
        {
            viz.subarray_low = state.current_subarray.low;
        }
        if (state.current_subarray.high < state.data.size())
        {
            viz.subarray_high = state.current_subarray.high;
        }
        
        // Marking recursion depth
        viz.recursion_depth = state.current_subarray.depth;
    }
} // namespace c2l::algorithms