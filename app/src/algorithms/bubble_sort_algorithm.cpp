//
// Created by Akhmad on 11/7/25.
//

#include "bubble_sort_algorithm.hpp"

#include "core/utils/logger/logger.hpp"

namespace c2l::algorithms
{
    BubbleSortAlgorithm::BubbleSortAlgorithm()
    {
        LOG_DEBUG("BubbleSortAlgorithm created");
    }

    void BubbleSortAlgorithm::initialize(const std::vector<int> &data)
    {
        m_original_data = data;
        reset();
        m_current_step_index = 0;

        generate_all_steps();
        LOG_INFO("BubbleSortAlgorithm initialized with {} elements", data.size());
    }

    bool BubbleSortAlgorithm::step_forward()
    {
        if (m_current_step_index < m_steps.size() - 1)
        {
            m_current_step_index++;
            notify_observers();
            return true;
        }

        return false;
    }

    bool BubbleSortAlgorithm::step_backward()
    {
        if (m_current_step_index > 0)
        {
            m_current_step_index--;
            notify_observers();
            return true;
        }

        return false;
    }

    void BubbleSortAlgorithm::reset()
    {
        m_current_step_index = 0;
        m_steps.clear();
    }

    std::vector<int> BubbleSortAlgorithm::get_original_data() const
    {
        return m_original_data;
    }

    void BubbleSortAlgorithm::populate_step_metadata(AlgorithmStep& out_step) const
    {
        const Step& current = m_steps[m_current_step_index];

        // Core data
        out_step.data = current.data;
        out_step.description = current.description;
        out_step.metadata.operation_type = current.operation_type;

        out_step.metadata.set("i", current.state.outer_loop_index, "Outer loop index");
        out_step.metadata.set("j", current.state.inner_loop_index, "Inner loop index");
        out_step.metadata.set("swapped", current.state.swapped_in_current_pass);
        out_step.metadata.set("comparisons", current.total_comparisons);
        out_step.metadata.set("swaps", current.total_swaps);
        out_step.metadata.set("sorted_count", current.state.sorted_elements);

        if (current.state.inner_loop_index < current.data.size())
            out_step.metadata.set("arr[j]", current.data[current.state.inner_loop_index]);

        if (current.state.inner_loop_index + 1 < current.data.size())
            out_step.metadata.set("arr[j+1]", current.data[current.state.inner_loop_index + 1]);

        // Tags ToDo: Create AlgorithmStepOperation type_to_string and string_to_type to use below
        if (current.operation_type == AlgorithmStepOperation::COMPARE)
            out_step.metadata.add_tag("COMPARISON");

        if (current.operation_type == AlgorithmStepOperation::SWAP)
        {
            out_step.metadata.add_tag("SWAP");
            out_step.metadata.add_tag("MODIFICATION");
        }

        if (current.operation_type == AlgorithmStepOperation::PASS_COMPLETE)
            out_step.metadata.add_tag("PASS_END");

        // Visualization
        out_step.visualization.highlighted_index = current.state.inner_loop_index;
        out_step.visualization.compared_index = current.state.inner_loop_index + 1;
        out_step.visualization.is_swap_step = current.state.swapped_in_current_pass;
        out_step.visualization.comparisons = current.total_comparisons;
        out_step.visualization.swaps = current.total_swaps;

        if (current.state.outer_loop_index > 0)
        {
            for (size_t k = current.data.size() - current.state.outer_loop_index;
                 k < current.data.size(); ++k)
            {
                out_step.visualization.additional_highlights.push_back(k);
            }
        }
    }


    size_t BubbleSortAlgorithm::get_step_count() const
    {
        return m_steps.size();
    }

    size_t BubbleSortAlgorithm::get_current_step_index() const
    {
        return m_current_step_index;
    }


    bool BubbleSortAlgorithm::is_complete() const
    {
        return m_current_step_index >= m_steps.size() - 1;
    }

    bool BubbleSortAlgorithm::is_steps_empty_or_invalid() const
    {
        if (m_steps.empty() || m_current_step_index >= m_steps.size())
            return true;

        return false;
    }

        void BubbleSortAlgorithm::generate_all_steps()
    {
        if (m_original_data.empty()) return;

        std::vector<int> data{m_original_data};
        const size_t n = data.size();

        // Initializing state
        BubbleSortState state{};
        state.sorted_elements = 0;

        size_t total_comparisons = 0;
        size_t total_swaps = 0;

        push_step(
            data,
            state,
            AlgorithmStepOperation::INIT,
            "Initial array state",
            total_comparisons,
            total_swaps);

        for (size_t i = 0; i < n - 1; ++i)
        {
            state.outer_loop_index = i;
            state.inner_loop_index = 0;
            state.swapped_in_current_pass = false;
            state.comparisons_in_pass = 0;
            state.swaps_in_pass = 0;

            // Outer loop step
            push_step(
                data,
                state,
                AlgorithmStepOperation::LOOP_OUTER,
                "Outer loop iteration: i = " + std::to_string(i),
            total_comparisons,
                total_swaps);

            for (size_t j = 0; j < n - i - 1; ++j)
            {
                state.inner_loop_index = j;

                // Inner loop step
                push_step(
                    data,
                    state,
                    AlgorithmStepOperation::LOOP_INNER,
                    "Inner loop: i = " + std::to_string(i) +
                                            ", j = " + std::to_string(j),
                total_comparisons,
                    total_swaps);

                // Comparison step
                total_comparisons++;
                state.comparisons_in_pass++;

                push_step(
                    data,
                    state,
                    AlgorithmStepOperation::COMPARE,
                    "Comparing arr[" + std::to_string(j) +
                                         "] = " + std::to_string(data[j]) +
                                         " with arr[" + std::to_string(j + 1) +
                                         "] = " + std::to_string(data[j + 1]),
                total_comparisons,
                    total_swaps);

                if (data[j] > data[j + 1])
                {
                    // Swap step
                    std::swap(data[j], data[j + 1]);
                    total_swaps++;

                    state.swaps_in_pass++;
                    state.swapped_in_current_pass = true;

                    push_step(
                        data,
                        state,
                        AlgorithmStepOperation::SWAP,
                        "Swapped arr[" + std::to_string(j) +
                            "] <-> arr[" + std::to_string(j + 1) + "]",
                        total_comparisons,
                        total_swaps);
                }
            }

            // Pass complete step
            state.sorted_elements = i + 1;

            push_step(
                data,
                state,
                AlgorithmStepOperation::PASS_COMPLETE,
                "Pass " + std::to_string(i + 1) + " complete. " +
                                   (state.swapped_in_current_pass ? "Swaps occurred." : "No swaps."),
                total_comparisons,
                total_swaps);
        }

        // Final sorted state
        state.outer_loop_index = n - 1;
        state.inner_loop_index = 0;
        state.sorted_elements = n;

        push_step(
            data,
            state,
            AlgorithmStepOperation::FINISHED,
            "Array is completely sorted",
            total_comparisons,
            total_swaps);

        LOG_DEBUG("Generated {} steps for bubble sort", m_steps.size());
    }

    void BubbleSortAlgorithm::push_step(
        const std::vector<int> &data,
        const BubbleSortState &state,
        AlgorithmStepOperation operation,
        std::string_view description,
        size_t total_comparison,
        size_t total_swaps)
    {
        Step step;
        step.data = data;
        step.state = state;
        step.operation_type = operation;
        step.description = std::string(description);
        step.total_comparisons = total_comparison;
        step.total_swaps = total_swaps;

        m_steps.push_back(std::move(step));
    }

    AlgorithmType BubbleSortAlgorithm::get_type() const
    {
        return AlgorithmType::BUBBLE_SORT;
    }


    std::string BubbleSortAlgorithm::get_time_complexity() const
    {
        return "O(n log n) average, O(n²) worst";
    }

    std::string BubbleSortAlgorithm::get_space_complexity() const
    {
        return "O(log n)";
    }
    std::string BubbleSortAlgorithm::get_description() const
    {
        return "Efficient sorting algorithm using divide-and-conquer with partitioning";
    }

    std::vector<std::string> BubbleSortAlgorithm::get_detailed_description() const
    {
        return {
            "Algorithm Analysis:",
            "──────────────────",
            "Time Complexity:",
            "• Best case:    O(n)   - when array is already sorted",
            "• Average case: O(n²)  - random data",
            "• Worst case:   O(n²)  - reverse sorted data",
            "",
            "Space Complexity: O(1) - in-place sorting",
            "",
            "Stability: Stable",
            "In-place:  Yes",
            "Adaptive:  Yes - detects already sorted arrays",
            "",
            "Key Characteristics:",
            "• Simple and intuitive implementation",
            "• Excellent for educational purposes",
            "• Efficient for small datasets (< 100 elements)",
            "• Adaptive: can detect already sorted arrays",
            "",
            "Performance Metrics:",
            "• Comparisons: (n-1) + (n-2) + ... + 1 = n(n-1)/2 ≈ O(n²)",
            "• Swaps: Up to n(n-1)/2 in worst case",
            "• Memory: Constant O(1) auxiliary space",
            "",
            "Optimization Note:",
            "The algorithm can be optimized by stopping early if no swaps",
            "occur in a complete pass (indicating the array is sorted)."
        };
    }

    std::vector<std::string> BubbleSortAlgorithm::get_variable_names() const
    {
        return {"i", "j", "swapped", "comparisons", "swaps", "sorted_count"};
    }

    std::unordered_map<std::string, std::string> BubbleSortAlgorithm::get_properties() const
    {
        return {
                {"Stability", "Stable"},
                {"In-place", "Yes"},
                {"Adaptive", "Yes"},
                {"Comparison-based", "Yes"},
                {"Online", "No"}
        };
    }




} // namespace c2l::algorithms