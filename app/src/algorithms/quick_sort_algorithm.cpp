//
// Created by Akhmad on 11/8/25.
//

#include "quick_sort_algorithm.hpp"
#include "core/utils/logger/logger.hpp"

namespace c2l::algorithms
{
    QuickSortAlgorithm::QuickSortAlgorithm()
    {
        LOG_DEBUG("QuickSortAlgorithm created");
    }

    void QuickSortAlgorithm::initialize(const std::vector<int>& data)
    {
        m_original_data = data;
        reset();
        generate_all_steps();
        LOG_INFO("QuickSortAlgorithm initialized with {} elements", data.size());
    }

    bool QuickSortAlgorithm::step_forward()
    {
        if (!m_steps.empty() && m_current_step_index < m_steps.size() - 1) {
            m_current_step_index++;
            return true;
        }
        return false;
    }

    bool QuickSortAlgorithm::step_backward()
    {
        if (m_current_step_index > 0) {
            m_current_step_index--;
            return true;
        }
        return false;
    }

    void QuickSortAlgorithm::reset()
    {
        m_current_step_index = 0;
        m_steps.clear();
    }

    std::vector<int> QuickSortAlgorithm::get_original_data() const
    {
        return m_original_data;
    }


    void QuickSortAlgorithm::populate_step_metadata(AlgorithmStep& out_step) const
    {
        const auto& current_step = m_steps[m_current_step_index];

        out_step.data = current_step.data;
        out_step.description = current_step.description;
        out_step.metadata.operation_type = current_step.operation_type;

        out_step.metadata.set("low", current_step.state.low, "low");
        out_step.metadata.set("high", current_step.state.high, "high");

        if (current_step.state.pivot_index)
            out_step.metadata.set("pivot_index", *current_step.state.pivot_index , "pivot index");
        if (current_step.state.pivot_value)
            out_step.metadata.set("pivot_value", *current_step.state.pivot_value , "pivot value");

        if (current_step.state.i)
            out_step.metadata.set("i", *current_step.state.i, "i");
        if (current_step.state.j)
            out_step.metadata.set("j", *current_step.state.j, "j");
        if (current_step.state.sorted_elements)
            out_step.metadata.set("sorted_count", *current_step.state.sorted_elements);

        out_step.metadata.set("is_partitioning", current_step.state.is_partitioning , "is_partitioning");
        out_step.metadata.set("is_swapping", current_step.state.is_swapping , "is_swapping");
        out_step.metadata.set("is_recursive_call", current_step.state.is_recursive_call, "is_recursive_call");

        if (current_step.state.i && *current_step.state.i < current_step.data.size())
        {
            out_step.metadata.set("arr[i]", current_step.data[*current_step.state.i]);
        }

        if (current_step.state.i &&
            *current_step.state.i + 1 < current_step.data.size())
        {
            out_step.metadata.set("arr[i+1]", current_step.data[*current_step.state.i + 1]);
        }

        if (current_step.state.j &&
            *current_step.state.j < current_step.data.size())
        {
            out_step.metadata.set("arr[j]", current_step.data[*current_step.state.j]);
        }

        if (current_step.state.pivot_index &&
            *current_step.state.pivot_index < current_step.data.size())
        {
            out_step.metadata.set("arr[pivot_index]", current_step.data[*current_step.state.pivot_index]);
        }

        if (current_step.state.pivot_value)
        {
            out_step.metadata.set("arr[pivot_value]", current_step.data[*current_step.state.pivot_value]);
        }

        // Tags
        if (current_step.operation_type == AlgorithmStepOperation::PARTITION)
            out_step.metadata.add_tag("PARTITION");

        if (current_step.operation_type == AlgorithmStepOperation::COMPARE)
            out_step.metadata.add_tag("COMPARISON");

        if (current_step.operation_type == AlgorithmStepOperation::SWAP)
        {
            out_step.metadata.add_tag("SWAP");
            out_step.metadata.add_tag("MODIFICATION");
        }

        if (current_step.operation_type == AlgorithmStepOperation::PASS_COMPLETE)
            out_step.metadata.add_tag("PASS_END");

        if (current_step.state.is_partitioning && current_step.state.pivot_index)
        {
            out_step.visualization.highlighted_index = *current_step.state.pivot_index;
            if (*current_step.state.j < current_step.data.size())
            {
                out_step.visualization.compared_index = *current_step.state.j;
            }
        }
    }

    size_t QuickSortAlgorithm::get_step_count() const
    {
        return m_steps.size();
    }

    size_t QuickSortAlgorithm::get_current_step_index() const
    {
        return m_current_step_index;
    }

    bool QuickSortAlgorithm::is_complete() const
    {
        if (m_steps.empty()) return true;
        return m_current_step_index >= m_steps.size() - 1;
    }

    bool QuickSortAlgorithm::is_steps_empty_or_invalid() const
    {
        if (m_steps.empty() || m_current_step_index >= m_steps.size())
            return true;

        return false;
    }

    void QuickSortAlgorithm::generate_all_steps()
    {
        if (m_original_data.empty()) return;

        // Start with a copy of original data
        std::vector<int> data = m_original_data;

        // Initializing state
        QuickSortState state{};
        state.sorted_elements = 0;

        size_t total_comparisons = 0;
        size_t total_swaps = 0;

        // Add initial step
        push_step(
            data,
            {},
            AlgorithmStepOperation::INIT,
            "Initial array",
            total_comparisons,
            total_swaps);

        // Perform recursive quick sort
        quick_sort(
            data,
            0,
            data.size() - 1,
            0,
            total_comparisons,
            total_swaps);

        push_step(
            data,
            {},
            AlgorithmStepOperation::FINISHED,
            "Array fully sorted",
            total_comparisons,
            total_swaps);

        LOG_DEBUG("Generated {} steps for quick sort", m_steps.size());
    }

    void QuickSortAlgorithm::quick_sort(
        std::vector<int>& data,
        size_t low,
        size_t high,
        size_t depth,
        size_t& comparisons,
        size_t& swaps)
    {
        QuickSortState state;
        state.low = low;
        state.high = high;
        state.is_recursive_call = true;
        state.recursion_depth = depth;

        push_step(
            data,
            state,
            AlgorithmStepOperation::RECURSIVE_CALL,
            "quick_sort(" + std::to_string(low) + ", " + std::to_string(high) + ")",
            comparisons,
            swaps
        );

        if (low >= high)
            return;

        const size_t pivot = partition(data, low, high, depth, comparisons, swaps);

        if (pivot > low)
        {
            quick_sort(data, low, pivot - 1, depth + 1, comparisons, swaps);
        }

        quick_sort(data, pivot + 1, high, depth + 1, comparisons, swaps);
    }

    size_t QuickSortAlgorithm::partition(
        std::vector<int> &data,
        size_t low,
        size_t high,
        size_t depth,
        size_t &comparisons,
        size_t &swaps)
    {
        int pivot = data[high];
        size_t i = low;

        for (size_t j = low; j < high; j++)
        {
            ++comparisons;

            QuickSortState compare;
            compare.low = low;
            compare.high = high;
            compare.pivot_index = high;
            compare.pivot_value = data[high];
            compare.i = i;
            compare.j = j;
            compare.is_partitioning = true;
            compare.recursion_depth = depth;

            push_step(
                data,
                compare,
                AlgorithmStepOperation::COMPARE,
                "Compare data[" + std::to_string(j) + "] with pivot[" + std::to_string(pivot) + "]",
                comparisons,
                swaps
            );

            if (data[j] <= pivot)
            {
                std::swap(data[i], data[j]);
                ++swaps;

                QuickSortState swap_state = compare;
                swap_state.is_swapping = true;

                push_step(
                    data,
                    swap_state,
                    AlgorithmStepOperation::SWAP,
                    "Swap data[" + std::to_string(i) + "] and data[" + std::to_string(j) + "]",
                    comparisons,
                    swaps
                );

                ++i;
            }
        }

        std::swap(data[i], data[high]);
        ++swaps;

        QuickSortState pivot_swap;
        pivot_swap.low = low;
        pivot_swap.high = high;
        pivot_swap.pivot_index = i;
        pivot_swap.is_swapping = true;
        pivot_swap.recursion_depth = depth;

        push_step(
            data,
            pivot_swap,
            AlgorithmStepOperation::PIVOT_PLACED,
            "Place pivot in final position",
            comparisons,
            swaps
        );

        return i;
    }


    void QuickSortAlgorithm::push_step(
        const std::vector<int>& data,
        const QuickSortState& state,
        const AlgorithmStepOperation operation,
        const std::string_view description,
        const size_t comparisons,
        const size_t swaps)
    {
        Step step;
        step.data = data;
        step.state = state;
        step.operation_type = operation;
        step.description = std::string(description);
        step.total_comparisons = comparisons;
        step.total_swaps = swaps;

        m_steps.push_back(std::move(step));
    }



    AlgorithmType QuickSortAlgorithm::get_type() const
    {
        return AlgorithmType::QUICK_SORT;
    }

    std::string QuickSortAlgorithm::get_time_complexity() const
    {
        return "O(n log n) average, O(n²) worst";
    }

    std::string QuickSortAlgorithm::get_space_complexity() const
    {
        return "O(log n)";
    }
    std::string QuickSortAlgorithm::get_description() const
    {
        return "Efficient sorting algorithm using divide-and-conquer with partitioning";
    }

    std::vector<std::string> QuickSortAlgorithm::get_detailed_description() const
    {
        return {
            "Algorithm Analysis:",
            "──────────────────",
            "Time Complexity:",
            "• Best case:    O(n log n) - balanced partitions",
            "• Average case: O(n log n) - random data",
            "• Worst case:   O(n²)      - already sorted or reverse sorted",
            "",
            "Space Complexity: O(log n) - recursion stack (average case)",
            "",
            "Stability: Not stable",
            "In-place:  Yes",
            "Adaptive:  No",
            "",
            "Key Characteristics:",
            "• Divide-and-conquer algorithm using partitioning",
            "• Very efficient for large datasets",
            "• Cache-friendly due to sequential access",
            "• Practical performance often better than other O(n log n) algorithms",
            "",
            "Performance Metrics:",
            "• Comparisons: ~1.39 n log n (average case)",
            "• Swaps: ~0.33 n log n (average case)",
            "• Memory: O(log n) stack space",
            "",
            "Optimization Notes:",
            "• Median-of-three pivot selection prevents worst-case performance",
            "• Insertion sort for small subarrays improves performance",
            "• Tail recursion optimization reduces stack depth"
        };
    }

    std::vector<std::string>QuickSortAlgorithm::get_variable_names() const
    {
        return {"low", "high", "pivot_index", "pivot_value", "i", "j",
                "is_partitioning", "is_swapping", "call_stack"};
    }

    std::unordered_map<std::string, std::string> QuickSortAlgorithm::get_properties() const
    {
        return {
            {"Stability", "Not stable"},
            {"In-place", "Yes"},
            {"Adaptive", "No"},
            {"Comparison-based", "Yes"},
            {"Online", "No"},
            {"Recursive", "Yes"},
            {"Divide & Conquer", "Yes"}
        };
    }

} // namespace c2l::algorithms