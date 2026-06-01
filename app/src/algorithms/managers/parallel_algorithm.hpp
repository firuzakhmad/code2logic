//
// Created by Akhmad on 5/29/26.
//

#ifndef CODE2LOGIC_PARALLEL_ALGORITHM_HPP
#define CODE2LOGIC_PARALLEL_ALGORITHM_HPP

#include <memory>
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <string>
#include <stddef.h>

#include "algorithms/core/atomic_performance_metrics.hpp"
#include "algorithms/core/i_simple_algorithm.hpp"
#include "algorithms/visualizers/i_algorithm_visualizer.hpp"
#include "algorithms/core/algorithm_types.hpp"
#include "algorithms/core/i_algorithm_metadata.hpp"


namespace c2l::algorithms
{
    struct ParallelAlgorithm
    {
        std::unique_ptr<ISimpleAlgorithm> algorithm;
        std::unique_ptr<IAlgorithmVisualizer> visualizer;
        const IAlgorithmMetadata *metadata          {nullptr};
        AtomicPerformanceMetrics metrics;
        AlgorithmType type                          {AlgorithmType::UNKNOWN};
        size_t id                                   {0};
        std::string name;

        std::vector<float> comparisons_history;
        std::vector<float> step_time_history_ms;

        // Thread synchronization
        std::mutex algorithm_mutex;
        std::condition_variable step_cv;
        std::atomic<bool> step_ready                {false};
        std::atomic<bool> step_completed            {true};

        bool is_valid() const noexcept
        {
            return algorithm != nullptr && metadata != nullptr;
        }

        void reset()
        {
            std::lock_guard<std::mutex> lock(algorithm_mutex);
            if (algorithm)
            {
                algorithm->reset();
            }

            if (visualizer)
            {
                visualizer.reset();
            }

            metrics.reset();
            step_ready = false;
            step_completed = true;
        }
    };
} // namespace c2l::algorithms

#endif //CODE2LOGIC_PARALLEL_ALGORITHM_HPP