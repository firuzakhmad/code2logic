#ifndef CODE2LOGIC_ALGORITHM_PERFORMANCE_METRICS
#define CODE2LOGIC_ALGORITHM_PERFORMANCE_METRICS

#include "algorithms/core/algorithm_step.hpp"

#include <chrono>
#include <stddef.h>
#include <atomic>

namespace c2l::algorithms
{
    /**
     * @brief Performance metrics with thread-safe updates
     */
    struct alignas(64) AtomicPerformanceMetrics
    {
        // Timing metrics
        std::atomic<std::chrono::microseconds::rep> execution_time        {0};
        std::atomic<std::chrono::microseconds::rep> initialization_time   {0};

        // Operation metrics
        std::atomic<size_t> comparison_count                       {0};
        std::atomic<size_t> swap_count                             {0};
        std::atomic<size_t> step_count                             {0};
        std::atomic<size_t> current_step                            {0};

        // Grid
        std::atomic<size_t> visited_node_count                     {0};
        std::atomic<size_t> explored_node_count                    {0};

        // Memory metrics
        std::atomic<size_t> memory_used                             {0};
        std::atomic<size_t> peak_memory                             {0};

        // Status
        std::atomic<bool> is_complete                               {false};
        std::atomic<float> progress                                 {0.0f};

        void update_from_step(const AlgorithmStep& step)
        {
            comparison_count = step.visualization.comparison_count;
            visited_node_count = step.visualization.visited_node_count;
            explored_node_count = step.visualization.explored_node_count;
            swap_count = step.visualization.swap_count;
            memory_used = step.data.size() * sizeof(int) + 
                         step.visualization.additional_highlights.size() * sizeof(size_t);
            peak_memory = std::max(peak_memory.load(), memory_used.load());
            ++step_count;
        }

        void reset()
        {
            execution_time = 0;
            initialization_time = 0;
            comparison_count = 0;
            swap_count = 0;
            visited_node_count = 0;
            explored_node_count = 0;
            step_count = 0;
            step_count = 0;
            current_step = 0;
            memory_used = 0;
            peak_memory = 0;
            is_complete = false;
            progress = 0.0;
        }
    };
 }// namespace c2l::algorithms

#endif // CODE2LOGIC_ALGORITHM_PERFORMANCE_METRICS