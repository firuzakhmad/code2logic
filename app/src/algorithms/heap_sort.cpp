#include "heap_sort.hpp"
#include "core/utils/logger/logger.hpp"
#include <algorithm>
#include <cmath>

namespace c2l::algorithms
{
    HeapSort::HeapSort(core::JsonConfigManager& json_config_manager)
        : JsonAlgorithmBase(json_config_manager, AlgorithmType::HEAP_SORT)
    {
        LOG_DEBUG("HeapSort created and metadata loaded from JSON");
    }

    void HeapSort::generate_all_steps()
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
        State state{};
        state.data = m_original_data;
        state.heap_size = m_original_data.size();
        state.phase = Phase::BUILD;
        state.comparisons = 0;
        state.swaps = 0;

        // Initialization step
        push_step(state, "init");

        // Phase 1: Build max heap
        build_heap(state);
        push_step(state, "build_heap_complete");

        // Phase 2: Extraction phase
        state.phase = Phase::EXTRACTION;
        push_step(state, "extraction_start");

        // Extract elements one by one
        for (size_t i = state.heap_size; i-- > 1;)
        {
            state.i = i;
            state.extract_index = i;
            
            // Swap root (maximum) with last element
            std::swap(state.data[0], state.data[i]);
            m_total_swaps++;
            state.swaps = m_total_swaps;
            
            push_step(state, "swap_extract");

            // Reduce heap size and heapify root
            state.heap_size = i;
            
            // Heapify the reduced heap
            heapify(state, 0);

            push_step(state, "extraction_complete");
        }

        // Final completion
        push_step(state, "completed");

        LOG_DEBUG(
            "Generated {} steps for heap sort", 
            m_steps.size()
        );
    }

    void HeapSort::build_heap(State& state)
    {
        // Start from the last non-leaf node
        for (size_t i = state.heap_size / 2; i-- > 0;)
        {
            state.i = i;
            state.build_heap_index = i;
            push_step(state, "build_heap_start");
            heapify(state, i);
        }
    }

    void HeapSort::heapify(State& state, size_t root)
    {
        size_t current_root = root;
        
        while (true)
        {
            state.root = current_root;
            state.largest = current_root;
            state.left = left_child_index(current_root);
            state.right = right_child_index(current_root);
            
            push_step(state, "heapify_start");
            
            bool made_swap = false;
            
            // Compare with left child
            if (state.left < state.heap_size)
            {
                m_total_comparisons++;
                state.comparisons = m_total_comparisons;
                
                push_step(state, "compare_left");
                if (state.data[state.left] > state.data[state.largest])
                {
                    state.largest = state.left;
                }
            }
            
            // Compare with right child
            if (state.right < state.heap_size)
            {
                m_total_comparisons++;
                state.comparisons = m_total_comparisons;
                
                push_step(state, "compare_right");
                if (state.data[state.right] > state.data[state.largest])
                {
                    state.largest = state.right;
                }
            }
            
            // If root is the largest, we're done
            if (state.largest == current_root)
                break;
            
            // Perform swap
            std::swap(state.data[current_root], state.data[state.largest]);
            m_total_swaps++;
            state.swaps = m_total_swaps;
            
            push_step(state, "swap_heapify");
            
            // Move down to the affected subtree
            current_root = state.largest;
            made_swap = true;
            
            if (!made_swap)
                break;
        }
    }

    void HeapSort::push_step(
        const State& state, 
        const std::string& operation_id)
    {
        auto step = create_step_from_state(state, operation_id);
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep HeapSort::create_step_from_state(
        const State& state, 
        const std::string& operation_id
    ) const
    {
        AlgorithmStep step;
        
        // Core data
        step.data = state.data;
        step.metadata.operation_id = operation_id;
        
        // Populate metadata using JSON-driven approach
        populate_step_metadata(step, state, operation_id);
        
        // Generate description using JSON template
        step.description = format_step_description(operation_id, step);
        
        // Update visualization data
        update_visualization_data(step, state, operation_id);
        
        return step;
    }

    void HeapSort::populate_step_metadata(
        AlgorithmStep& step, 
        const State& state, 
        const std::string& operation_id
    ) const
    {
        const size_t n = state.data.size();
        
        // Core heap variables (matching JSON variable names)
        step.metadata.set(
            "heap_size", 
            state.heap_size, 
            "Current heap size (unsorted portion)"
        );
        step.metadata.set(
            "i", 
            state.i, 
            "Current loop index"
        );
        step.metadata.set(
            "root", 
            state.root, 
            "Current root index being heapified"
        );
        step.metadata.set(
            "largest", 
            state.largest, 
            "Index of largest element among root and children"
        );
        step.metadata.set(
            "left", 
            state.left, 
            "Left child index"
        );
        step.metadata.set(
            "right", 
            state.right, 
            "Right child index"
        );
        step.metadata.set(
            "build_heap_index", 
            state.build_heap_index, 
            "Current index during heap construction"
        );
        step.metadata.set(
            "extract_index", 
            state.extract_index, 
            "Current index during extraction"
        );
        step.metadata.set(
            "size", 
            n, 
            "Total array size"
        );
        step.metadata.set(
            "comparisons", 
            state.comparisons, 
            "Total comparisons performed"
        );
        step.metadata.set(
            "swaps", 
            state.swaps, 
            "Total swaps performed"
        );
        step.metadata.set(
            "phase", 
            phase_to_string(state.phase), 
            "Current algorithm phase"
        );
        
        // Array values with bounds checking
        if (state.root < n)
        {
            step.metadata.set(
                "arr[root]", 
                state.data[state.root],
                std::string("Value at root index ") + std::to_string(state.root)
            );
        }
        
        if (state.largest < n)
        {
            step.metadata.set(
                "arr[largest]", 
                state.data[state.largest],
                std::string("Value at largest index ") + std::to_string(state.largest)
            );
        }
        
        if (state.left < n)
        {
            step.metadata.set(
                "arr[left]", state.data[state.left],
                std::string("Value at left child index ") + std::to_string(state.left)
            );
        }
        
        if (state.right < n)
        {
            step.metadata.set(
                "arr[right]", state.data[state.right],
                std::string("Value at right child index ") + std::to_string(state.right)
            );
        }
        
        // Add context for specific operations
        if (operation_id == "swap_heapify")
        {
            if (state.root < n && state.largest < n)
            {
                step.metadata.set(
                    "swapped_root_value", 
                    state.data[state.largest],
                    "Value that was swapped up to root"
                );
                step.metadata.set(
                    "swapped_child_value", 
                    state.data[state.root],
                    "Value that was swapped down"
                );
            }
        }
        else if (operation_id == "swap_extract")
        {
            if (state.extract_index < n)
            {
                step.metadata.set(
                    "arr[0]", state.data[0], 
                    "Heap root (maximum value)"
                );
                step.metadata.set(
                    "new_heap_size", 
                    state.heap_size, 
                    "New heap size after extraction"
                );
            }
        }
        else if (operation_id == "extraction_complete")
        {
            step.metadata.set(
                "new_heap_size", 
                state.heap_size, 
                "New heap size after extraction"
            );
            if (state.extract_index < n && 
                state.extract_index > 0)
            {
                step.metadata.set(
                    "sorted_value", 
                    state.data[state.extract_index],
                    "Value placed in sorted position"
                );
            }
        }
        else if (operation_id == "build_heap_start")
        {
            size_t remaining = state.build_heap_index + 1;
            step.metadata.set(
                "remaining_nodes", 
                remaining, 
                "Number of nodes remaining to heapify"
            );
        }
        
        // Adding tags from step mapping
        auto tags = get_step_tags(operation_id);
        for (const auto& tag : tags)
        {
            step.metadata.add_tag(tag);
        }
    }

    void HeapSort::update_visualization_data(
        AlgorithmStep& step, 
        const State& state, 
        const std::string& operation_id
    ) const
    {
        auto& viz = step.visualization;
        
        // Basic metrics
        viz.comparison_count = state.comparisons;
        viz.swap_count = state.swaps;
        
        // Clear previous highlights
        viz.highlighted_index = std::numeric_limits<size_t>::max();
        viz.compared_index = std::numeric_limits<size_t>::max();
        viz.additional_highlights.clear();
        
        // Set heap boundaries
        if (state.heap_size > 0)
        {
            viz.heap_size = state.heap_size;
            viz.subarray_low = 0;
            viz.subarray_high = state.heap_size - 1;
        }
        
        // Set visualization based on operation type
        if (operation_id == "init")
        {
            // Show entire array as unsorted
            viz.subarray_low = 0;
            viz.subarray_high = state.data.size() - 1;
        }
        else if (operation_id == "build_heap_start")
        {
            // Highlight the node being heapified
            if (state.build_heap_index < state.data.size())
            {
                viz.highlighted_index = state.build_heap_index;
                viz.additional_highlights.push_back(state.build_heap_index);
            }
            // Mark the heap region
            viz.subarray_low = 0;
            viz.subarray_high = state.heap_size - 1;
        }
        else if (operation_id == "heapify_start")
        {
            // Highlight the root being heapified
            if (state.root < state.heap_size)
            {
                viz.highlighted_index = state.root;
                viz.additional_highlights.push_back(state.root);
            }
            // Also highlight children if they exist
            if (state.left < state.heap_size)
            {
                viz.additional_highlights.push_back(state.left);
            }
            if (state.right < state.heap_size)
            {
                viz.additional_highlights.push_back(state.right);
            }
            viz.heapify_root = state.root;
        }
        else if (operation_id == "compare_left")
        {
            // Highlight root and left child being compared
            if (state.root < state.heap_size)
            {
                viz.highlighted_index = state.root;
            }
            if (state.left < state.heap_size)
            {
                viz.compared_index = state.left;
                viz.additional_highlights.push_back(state.left);
            }
            viz.is_swap_step = false;
        }
        else if (operation_id == "compare_right")
        {
            // Highlight largest so far and right child being compared
            if (state.largest < state.heap_size)
            {
                viz.highlighted_index = state.largest;
            }
            if (state.right < state.heap_size)
            {
                viz.compared_index = state.right;
                viz.additional_highlights.push_back(state.right);
            }
            viz.is_swap_step = false;
        }
        else if (operation_id == "swap_heapify")
        {
            // Highlight both elements being swapped during heapify
            if (state.root < state.heap_size)
            {
                viz.highlighted_index = state.root;
                viz.additional_highlights.push_back(state.root);
            }
            if (state.largest < state.heap_size && state.largest != state.root)
            {
                viz.compared_index = state.largest;
                viz.additional_highlights.push_back(state.largest);
            }
            viz.is_swap_step = true;
        }
        else if (operation_id == "build_heap_complete")
        {
            // Mark the entire heap as valid max heap
            for (size_t k = 0; k < state.heap_size && k < state.data.size(); ++k)
            {
                viz.additional_highlights.push_back(k);
            }
            viz.subarray_low = 0;
            viz.subarray_high = state.heap_size - 1;
        }
        else if (operation_id == "extraction_start")
        {
            // Show current state before extraction
            viz.subarray_low = 0;
            viz.subarray_high = state.heap_size - 1;
        }
        else if (operation_id == "swap_extract")
        {
            // Highlight root and last element being swapped
            if (state.heap_size > 0)
            {
                viz.highlighted_index = 0;
                viz.additional_highlights.push_back(0);
            }
            if (state.extract_index < state.data.size())
            {
                viz.compared_index = state.extract_index;
                viz.additional_highlights.push_back(state.extract_index);
            }
            viz.is_swap_step = true;
        }
        else if (operation_id == "extraction_complete")
        {
            // Mark the newly sorted element at the end
            if (state.extract_index < state.data.size())
            {
                viz.additional_highlights.push_back(state.extract_index);
            }
            // Mark the reduced heap region
            if (state.heap_size > 0)
            {
                viz.subarray_low = 0;
                viz.subarray_high = state.heap_size - 1;
            }
        }
        else if (operation_id == "completed")
        {
            // Mark entire array as sorted
            for (size_t k = 0; k < state.data.size(); ++k)
            {
                viz.additional_highlights.push_back(k);
            }
            viz.subarray_low = 0;
            viz.subarray_high = state.data.size() - 1;
            viz.is_complete = true;
        }
        
        // Mark sorted portion for extraction phase steps
        if (state.phase == Phase::EXTRACTION && 
            operation_id != "init" && 
            operation_id != "build_heap_start" &&
            operation_id != "build_heap_complete" &&
            operation_id != "extraction_start")
        {
            size_t sorted_start = state.heap_size;
            for (size_t k = sorted_start; k < state.data.size(); ++k)
            {
                if (std::find(viz.additional_highlights.begin(), 
                              viz.additional_highlights.end(), k) == viz.additional_highlights.end())
                {
                    viz.additional_highlights.push_back(k);
                }
            }
        }
        
        // Store heap structure for tree visualization
        if (state.heap_size > 0)
        {
            viz.heap_structure.clear();
            for (size_t i = 0; i < state.heap_size; ++i)
            {
                viz.heap_structure.push_back(i);
            }
        }
    }

    std::string HeapSort::phase_to_string(Phase phase) const
    {
        switch (phase)
        {
            case Phase::BUILD:
                return "build_heap";
            case Phase::EXTRACTION:
                return "extraction";
            default:
                return "unknown";
        }
    }

} // namespace c2l::algorithms