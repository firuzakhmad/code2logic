//
// Created by Akhmad on 3/12/26.
//

#include "parallel_comparison_manager.hpp"

#include <random>

#include "algorithms/bubble_sort.hpp"
#include "algorithms/quick_sort.hpp"
#include "algorithms/visualizers/array_based_visualizer.hpp"
#include "core/utils/variables.hpp"

namespace c2l::algorithms
{
    ParallelComparisonManager::ParallelComparisonManager(
        core::ThreadManager &thread_manager,
        AlgorithmRegistry& algorithm_registry)
            : m_thread_manager{thread_manager}
            , m_algorithm_registry{algorithm_registry}
    {}

    ParallelComparisonManager::~ParallelComparisonManager()
    {
        stop_background_playback();

        // Waiting for any pending tasks
        {
            std::lock_guard<std::mutex> lock(m_tasks_mutex);
            for (auto& task : m_parallel_tasks)
            {
                if (task.valid())
                {
                    task.wait_for(std::chrono::milliseconds(10));
                }
            }
        }

        if (m_left.algorithm)
        {
            m_left.algorithm->remove_observer(this);
        }

        if (m_right.algorithm)
        {
            m_right.algorithm->remove_observer(this);
        }
    }

    bool ParallelComparisonManager::set_algorithm_left(
        AlgorithmType type)
    {
        return initialize_algorithm(m_left, type, 0);
    }

    bool ParallelComparisonManager::set_algorithm_right(
        AlgorithmType type)
    {
        return initialize_algorithm(m_right, type, 1);
    }

    bool ParallelComparisonManager::initialize_algorithm(
        ParallelAlgorithm &algorithm,
        AlgorithmType type,
        size_t id)
    {
        std::unique_lock lock(m_comparison_mutex);

        // Check if algorithm exists in registry
        if (!m_algorithm_registry.has_algorithm(type))
        {
            LOG_ERROR(
                "Algorithm type '{}' not registered in registry",
                algorithm_display_name(type)
            );
            return false;
        }

        // Removing old observer if exists
        if (algorithm.algorithm)
        {
            algorithm.algorithm->remove_observer(this);
        }

        algorithm.algorithm = m_algorithm_registry.create_algorithm(type);
        algorithm.visualizer = m_algorithm_registry.create_visualizer(type);

        if (!algorithm.algorithm || !algorithm.visualizer)
        {
            LOG_ERROR(
                "Failed to create algorithm/visualizer instance for type: {}",
                algorithm_display_name(type)
            );
            return false;
        }

        algorithm.metadata = algorithm.algorithm->metadata();
        algorithm.type = type;
        algorithm.name = algorithm_display_name(type);
        algorithm.id = id;

        if (!algorithm.metadata || !algorithm.metadata->is_valid())
        {
            LOG_ERROR(
                "Invalid metadata for algorithm: {}",
                algorithm.name
            );
            algorithm.algorithm.reset();
            return false;
        }

        algorithm.algorithm->add_observer(this);

        // Initializing with current data if available
        if (!m_current_data.empty())
        {
            algorithm.algorithm->initialize(m_current_data);
            algorithm.metrics.step_count = algorithm.algorithm->get_step_count();
        }

        if (algorithm.visualizer)
        {
            algorithm.visualizer->initialize(
                algorithm.algorithm.get(),
                algorithm.metadata,
                false
            );
        }

        LOG_INFO(
            "Set algorithm {} : {}",
            id == 0 ? "left" : "right",
            algorithm.name
        );
        return true;
    }

    void ParallelComparisonManager::set_shared_data(
        const std::vector<int> &data)
    {
        stop_background_playback();

        std::unique_lock lock(m_comparison_mutex);

        m_current_data = data;

        // Reset metrics BEFORE spawning tasks so we start clean
        m_left.metrics.reset();
        m_right.metrics.reset();

        std::vector<std::future<void>> init_tasks;

        if (m_left.is_valid())
        {
            auto left_future = m_thread_manager.enqueue_task(
                core::ThreadManager::ThreadType::COMPUTE,
                [this, data]()
                {
                    // Capture start INSIDE the lambda so we measure only
                    // generate_all_steps(), not thread pool queue latency
                    const auto t_start = std::chrono::high_resolution_clock::now();
                    m_left.algorithm->initialize(data);
                    const auto t_end = std::chrono::high_resolution_clock::now();

                    m_left.metrics.initialization_time =
                        std::chrono::duration_cast<std::chrono::microseconds>(
                            t_end - t_start).count();
                    m_left.metrics.step_count   = m_left.algorithm->get_step_count();
                    m_left.metrics.peak_memory   = m_left.algorithm->get_peak_memory_bytes();
                    m_left.metrics.execution_time =
                        m_left.algorithm->get_algorithm_time_us();
                }
            );
            init_tasks.push_back(std::move(left_future));
        }

        if (m_right.is_valid())
        {
            auto right_future = m_thread_manager.enqueue_task(
                core::ThreadManager::ThreadType::COMPUTE,
                [this, data]()
                {
                    const auto t_start = std::chrono::high_resolution_clock::now();
                    m_right.algorithm->initialize(data);
                    const auto t_end = std::chrono::high_resolution_clock::now();

                    m_right.metrics.initialization_time =
                        std::chrono::duration_cast<std::chrono::microseconds>(
                            t_end - t_start).count();
                    m_right.metrics.step_count   = m_right.algorithm->get_step_count();
                    m_right.metrics.peak_memory   = m_right.algorithm->get_peak_memory_bytes();
                    m_right.metrics.execution_time =
                        m_right.algorithm->get_algorithm_time_us();
                }
            );
            init_tasks.push_back(std::move(right_future));
        }

        for (auto& task : init_tasks)
        {
            if (task.valid())
                task.wait();
        }

        LOG_DEBUG(
            "Shared data set: {} elements | left={}us {}steps | right={}us {}steps",
            data.size(),
            m_left.metrics.execution_time.load(),
            m_left.metrics.step_count.load(),
            m_right.metrics.execution_time.load(),
            m_right.metrics.step_count.load()
        );
    }

    void ParallelComparisonManager::generate_random_data(
        const size_t size,
        const int max_value)
    {
        std::vector<int> data;
        data.reserve(size);

        thread_local std::random_device rd;
        thread_local std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, max_value);

        for (size_t i = 0; i < size; ++i)
        {
            data.push_back(dis(gen));
        }

        set_shared_data(data);
    }

    void ParallelComparisonManager::play()
    {
        if (is_complete())
        {
            reset();
        }

        m_is_playing = true;
        m_is_paused = false;

        m_playback_start_time = std::chrono::high_resolution_clock::now();

        start_background_playback();

        LOG_DEBUG("Parallel comparison playback started");
    }

    void ParallelComparisonManager::pause()
    {
        m_is_playing = false;
        m_is_paused = true;

        stop_background_playback();

        LOG_DEBUG("Parallel comparison paused");
    }

    void ParallelComparisonManager::stop()
    {
        stop_background_playback();

        m_is_playing = false;
        m_is_paused = false;

        std::unique_lock lock(m_comparison_mutex);

        // Resetting both algorithms in parallel
        std::vector<std::future<void>> reset_tasks;

        if (m_left.is_valid())
        {
            auto left_future = m_thread_manager.enqueue_task(
                core::ThreadManager::ThreadType::COMPUTE,
                [this]()
                {
                    std::lock_guard<std::mutex> lock(m_left.algorithm_mutex);
                    m_left.algorithm->reset();
                }
            );
            reset_tasks.push_back(std::move(left_future));
        }

        if (m_right.is_valid())
        {
            auto right_future = m_thread_manager.enqueue_task(
                core::ThreadManager::ThreadType::COMPUTE,
                [this]()
                {
                    std::lock_guard<std::mutex> lock(m_right.algorithm_mutex);
                    m_right.algorithm->reset();
                }
            );
            reset_tasks.push_back(std::move(right_future));
        }

        // Waiting for resets
        for (auto& task : reset_tasks)
        {
            if (task.valid())
            {
                task.wait();
            }
        }

        update_metrics(m_left, m_left.algorithm->get_current_step());
        update_metrics(m_right, m_right.algorithm->get_current_step());

        LOG_DEBUG("Parallel comparison stopped");
    }

    void ParallelComparisonManager::step_forward()
    {
        execute_parallel_step(true);
    }

    void ParallelComparisonManager::step_backward()
    {
        execute_parallel_step(false);
    }

    void ParallelComparisonManager::execute_parallel_step(bool forward)
    {
        if (is_complete())
        {
            return;
        }

        // Submitting parallel tasks for both algorithms
        m_pending_steps = 0;

        if (m_left.is_valid() && !m_left.algorithm->is_complete())
        {
            ++m_pending_steps;
            submit_step_task(m_left, forward);
        }

        if (m_right.is_valid() && !m_right.algorithm->is_complete())
        {
            ++m_pending_steps;
            submit_step_task(m_right, forward);
        }

        // Waiting for both steps to complete
        wait_for_all_steps();

        auto step_end = std::chrono::high_resolution_clock::now();

        // Update execution time
        if (m_is_playing)
        {
            m_total_playback_time =
                std::chrono::duration_cast<std::chrono::microseconds>(
                    step_end - m_playback_start_time).count();
        }
    }

    void ParallelComparisonManager::submit_step_task(
        ParallelAlgorithm& algo, 
        bool forward)
    {
        auto future = m_thread_manager.enqueue_task(
            core::ThreadManager::ThreadType::COMPUTE,
            [this, &algo, forward]()
            {
                std::lock_guard<std::mutex> lock(algo.algorithm_mutex);

                // Measuring step execution time
                auto step_start = std::chrono::high_resolution_clock::now();

                if (forward && !algo.algorithm->is_complete())
                {
                    algo.algorithm->step_forward();
                }
                else if (!forward && algo.algorithm->get_current_step_index() > 0)
                {
                    algo.algorithm->step_backward();
                }

                auto step_end = std::chrono::high_resolution_clock::now();
                auto step_duration = std::chrono::duration_cast<std::chrono::microseconds>(
                    step_end - step_start
                );

                // Updating metrics
                auto current_step = algo.algorithm->get_current_step();
                algo.metrics.update_from_step(current_step);
                algo.metrics.execution_time += step_duration.count();
                algo.metrics.current_step = algo.algorithm->get_current_step_index();
                algo.metrics.is_complete = algo.algorithm->is_complete();

                // Record history for live graphs
                algo.comparisons_history.push_back(
                    static_cast<float>(algo.metrics.comparison_count.load()));
                algo.step_time_history_ms.push_back(
                    static_cast<float>(step_duration.count()) / 1000.0f);

                if (algo.metrics.step_count > 0)
                {
                    algo.metrics.progress = static_cast<float>(algo.metrics.current_step) /
                                                static_cast<float>(algo.metrics.step_count - 1);
                }

                algo.step_completed = true;

                // Signaling completion
                {
                    std::lock_guard<std::mutex> step_lock(m_step_mutex);
                    --m_pending_steps;
                }
                m_step_completion_cv.notify_one();
            }
        );

        // Storing task
        {
            std::lock_guard<std::mutex> lock(m_tasks_mutex);
            m_parallel_tasks.push_back(std::move(future));
        }
    }

    void ParallelComparisonManager::wait_for_all_steps()
    {
        std::unique_lock<std::mutex> lock(m_step_mutex);
        m_step_completion_cv.wait(lock, [this]() {
            return m_pending_steps == 0;
        });
    }

    void ParallelComparisonManager::start_background_playback()
    {
        if (m_parallel_execution_active.exchange(true))
        {
            return;
        }

        // Launch background playback task using ThreadManager
        auto future = m_thread_manager.enqueue_task(
            core::ThreadManager::ThreadType::IO,
            [this]()
            {
                background_playback_loop();
            }
        );

        {
            std::lock_guard<std::mutex> lock(m_tasks_mutex);
            m_parallel_tasks.push_back(std::move(future));
        }
    }

    void ParallelComparisonManager::stop_background_playback()
    {
        m_parallel_execution_active = false;
        m_is_playing = false;
    }

    void ParallelComparisonManager::background_playback_loop()
    {
        LOG_DEBUG("Background playback loop started");

        auto last_time = std::chrono::high_resolution_clock::now();
        constexpr double step_interval = 0.5; // 500ms per step
        double accumulated_time = 0.0;

        while (m_parallel_execution_active &&
               m_is_playing &&
               !m_is_paused &&
               !is_complete())
        {
            auto current_time = std::chrono::high_resolution_clock::now();
            double delta_time = std::chrono::duration<double>(
                current_time - last_time).count();
            last_time = current_time;

            accumulated_time += delta_time * static_cast<double>(m_speed);

            while (accumulated_time >= step_interval &&
                   m_parallel_execution_active &&
                   m_is_playing &&
                   !m_is_paused &&
                   !is_complete())
            {
                execute_parallel_step(true);
                accumulated_time -= step_interval;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        LOG_DEBUG("Background playback loop ended");
    }

    void ParallelComparisonManager::reset()
    {
        stop_background_playback();

        std::unique_lock lock(m_comparison_mutex);

        if (!m_current_data.empty())
        {
            // Parallel reset with re-initialization
            std::vector<std::future<void>> reset_tasks;

            if (m_left.is_valid())
            {
                auto left_future = m_thread_manager.enqueue_task(
                    core::ThreadManager::ThreadType::COMPUTE,
                    [this]() 
                    {
                        std::lock_guard<std::mutex> lock(m_left.algorithm_mutex);
                        m_left.algorithm->initialize(m_current_data);
                    }
                );
                reset_tasks.push_back(std::move(left_future));
            }

            if (m_right.is_valid())
            {
                auto right_future = m_thread_manager.enqueue_task(
                    core::ThreadManager::ThreadType::COMPUTE,
                    [this]() 
                    {
                        std::lock_guard<std::mutex> lock(m_right.algorithm_mutex);
                        m_right.algorithm->initialize(m_current_data);
                    }
                );
                reset_tasks.push_back(std::move(right_future));
            }

            // Waiting for resets
            for (auto& task : reset_tasks)
            {
                if (task.valid())
                {
                    task.wait();
                }
            }
        }

        m_left.metrics.reset();
        m_right.metrics.reset();
        m_left.comparisons_history.clear();
        m_right.comparisons_history.clear();
        m_left.step_time_history_ms.clear();
        m_right.step_time_history_ms.clear();

        if (m_left.is_valid())
            m_left.metrics.step_count = m_left.algorithm->get_step_count();
        if (m_right.is_valid())
            m_right.metrics.step_count = m_right.algorithm->get_step_count();

        m_total_playback_time = 0;

        LOG_DEBUG("Parallel comparison reset");
    }

    void ParallelComparisonManager::set_speed(float speed)
    {
        m_speed = std::clamp(
            speed,
            ALGORITHM_COMPUTATION_MIN_SPEED,
            ALGORITHM_COMPUTATION_MAX_SPEED
        );
    }

    bool ParallelComparisonManager::is_complete() const
    {
        bool left_complete = !m_left.is_valid() 
                            || m_left.algorithm->is_complete();
        bool right_complete = !m_right.is_valid() 
                            || m_right.algorithm->is_complete();

        return left_complete && right_complete;
    }

    void ParallelComparisonManager::on_step_changed()
    {
        // This is called from algorithm threads
    }

    void ParallelComparisonManager::on_algorithm_completed()
    {
        LOG_DEBUG("Algorithm completed");

        if (is_complete())
        {
            m_is_playing = false;
            LOG_INFO("Both algorithms completed");
        }
    }

    void ParallelComparisonManager::update_metrics(
        ParallelAlgorithm& algo, 
        const AlgorithmStep& step)
    {
        algo.metrics.update_from_step(step);
        algo.metrics.current_step = algo.algorithm->get_current_step_index();
        algo.metrics.is_complete = algo.algorithm->is_complete();
        algo.metrics.visited_node_count = algo.visualizer->get_total_visited_nodes();
        algo.metrics.explored_node_count = algo.visualizer->get_total_explored_nodes();

        if (algo.metrics.step_count > 0)
        {
            algo.metrics.progress = static_cast<float>(algo.metrics.current_step) /
                                        static_cast<float>(algo.metrics.step_count - 1);
        }
    }

    ParallelComparisonManager::ParallelComparisonResult
    ParallelComparisonManager::get_parallel_comparison_result() const
    {
        ParallelComparisonResult result;

        // Left algorithm results
        result.left.total_time = std::chrono::microseconds(
            m_left.metrics.execution_time.load()
        );
        result.left.steps = m_left.metrics.step_count.load();
        result.left.comparisons = m_left.metrics.comparison_count.load();
        result.left.swaps = m_left.metrics.swap_count.load();
        result.left.visited_node_count = m_left.metrics.visited_node_count.load();
        result.left.explored_node_count = m_left.metrics.explored_node_count.load();

        // Right algorithm results
        result.right.total_time = std::chrono::microseconds(
            m_right.metrics.execution_time.load()
        );
        result.right.steps = m_right.metrics.step_count.load();
        result.right.comparisons = m_right.metrics.comparison_count.load();
        result.right.swaps = m_right.metrics.swap_count.load();
        result.right.visited_node_count = m_right.metrics.visited_node_count.load();
        result.right.explored_node_count = m_right.metrics.explored_node_count.load();

        // Calculate average step times
        if (result.left.steps > 0)
        {
            result.left.avg_step_time_ms = static_cast<double>(result.left.total_time.count()) /
                              (1000.0 * static_cast<double>(m_left.metrics.step_count.load()));
        }

        if (result.right.steps > 0)
        {
            result.right.avg_step_time_ms = static_cast<double>(result.right.total_time.count()) /
                               (1000.0 * static_cast<double>(m_right.metrics.step_count.load()));
        }

        // Calculate scores
        if (result.left.total_time.count() > 0 && result.right.total_time.count() > 0)
        {
            // speed_score: faster algorithm gets a score closer to 1.0
            result.left.speed_score = 1.0 /
                (1.0 + static_cast<double>(result.left.total_time.count()) / 1000000.0);
            result.right.speed_score = 1.0 /
                (1.0 + static_cast<double>(result.right.total_time.count()) / 1000000.0);

            // efficiency_score: comparisons per step — lower means fewer redundant
            // comparisons per unit of work, which is the meaningful metric
            result.left.efficiency_score = result.left.steps > 0
                ? static_cast<double>(result.left.comparisons) /
                  static_cast<double>(result.left.steps)
                : 0.0;
            result.right.efficiency_score = result.right.steps > 0
                ? static_cast<double>(result.right.comparisons) /
                  static_cast<double>(result.right.steps)
                : 0.0;

            // Parallel efficiency (how well they utilized parallel execution)
            double total_sequential_time = static_cast<double>(result.left.total_time.count()) +
                                          static_cast<double>(result.right.total_time.count());
            double max_parallel_time = std::max(
                static_cast<double>(result.left.total_time.count()),
                static_cast<double>(result.right.total_time.count()));
            result.parallel_efficiency = (total_sequential_time / max_parallel_time) / 2.0;

            // Determine winner
            if (result.left.total_time < result.right.total_time)
            {
                result.winner = "left";
                result.performance_ratio =
                    static_cast<double>(result.right.total_time.count()) /
                    static_cast<double>(result.left.total_time.count());
            }
            else if (result.right.total_time < result.left.total_time)
            {
                result.winner = "right";
                result.performance_ratio =
                    static_cast<double>(result.left.total_time.count()) /
                    static_cast<double>(result.right.total_time.count());
            }
        }

        return result;
    }

    float ParallelComparisonManager::get_speed() const noexcept
    {
        return m_speed.load();
    }

    // State queries
    bool ParallelComparisonManager::is_playing() const noexcept
    {
        return m_is_playing;
    }
    bool ParallelComparisonManager::is_paused() const noexcept
    {
        return m_is_paused;
    }

    const ParallelAlgorithm&
    ParallelComparisonManager::get_left_algorithm() const noexcept
    {
        return m_left;
    }
    const ParallelAlgorithm&
    ParallelComparisonManager::get_right_algorithm() const noexcept
    {
        return m_right;
    }
    const std::vector<int>&
    ParallelComparisonManager::get_current_data() const noexcept
    {
        return m_current_data;
    }


} // namespace c2l::algorithms