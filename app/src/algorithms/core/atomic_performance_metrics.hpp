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
        std::atomic<size_t> total_comparisons                       {0};
        std::atomic<size_t> total_swaps                             {0};
        std::atomic<size_t> total_steps                             {0};
        std::atomic<size_t> current_step                            {0};
        std::atomic<size_t> step_count                              {0};

        // Memory metrics
        std::atomic<size_t> memory_used                             {0};
        std::atomic<size_t> peak_memory                             {0};

        // Status
        std::atomic<bool> is_complete                               {false};
        std::atomic<float> progress                                 {0.0f};

        void update_from_step(const AlgorithmStep& step)
        {
            total_comparisons = step.visualization.comparisons;
            total_swaps = step.visualization.swaps;
            memory_used = step.data.size() * sizeof(int) + 
                         step.visualization.additional_highlights.size() * sizeof(size_t);
            peak_memory = std::max(peak_memory.load(), memory_used.load());
            ++step_count;
        }

        void reset()
        {
            execution_time = 0;
            initialization_time = 0;
            total_comparisons = 0;
            total_swaps = 0;
            total_steps = 0;
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