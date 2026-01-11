#include "core/utils/thread_manager/thread_manager.hpp"
#include "core/utils/logger/logger.hpp"
#include "core/utils/assert/assert.hpp"

#include <algorithm>
#include <utility>

namespace c2l::core
{
    const char* ThreadManager::thread_type_to_string(const ThreadType type)
    {
        switch (type) {
            case ThreadType::MAIN:          return "MAIN";
            case ThreadType::SIMULATION:    return "SIMULATION";
            case ThreadType::IO:            return "IO";
            case ThreadType::BACKGROUND:    return "BACKGROUND";

            default:                        return "UNKNOWN";
        }
    }

    ThreadManager::ThreadManager(Config  config)
        : m_config(std::move(config))
        , m_start_time(std::chrono::steady_clock::now())
    {
        C2L_ASSERT(m_config.min_workers > 0, "Minimum workers must be > 0");
        C2L_ASSERT(m_config.max_workers >= m_config.min_workers,
                   "Max workers must be >= min workers");

        LOG_INFO("ThreadManager constructing with config: min={}, max={}, hw_concurrency={}",
                 m_config.min_workers, m_config.max_workers,
                 std::thread::hardware_concurrency());

        // Initializing task queues
        for (auto type : get_available_thread_types())
        {
            m_task_queues[type] = std::queue<Task>();
        }

        m_running.store(true, std::memory_order_release);
        m_emergency_stop.store(false, std::memory_order_release);
        m_shutdown_requested.store(false, std::memory_order_release);
        m_paused.store(false, std::memory_order_release);
        m_total_queued_tasks.store(0, std::memory_order_release);
        m_active_worker_count.store(0, std::memory_order_release);

        // Initializing worker threads
        initialize_workers();

        // Verifying workers started
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Giving workers time to start
        LOG_INFO("ThreadManager initialized with {} active workers (expected: {})",
                 m_active_worker_count.load(), m_workers.size());

        // Starting performance monitor if enabled
        if (m_config.enable_performance_monitoring)
        {
            m_monitor_running.store(true, std::memory_order_release);
            m_monitor_thread = std::thread([this]()
            {
                monitor_performance();
            });

            LOG_DEBUG("Performance monitor started");
        }

        // Quick test to verify functionality
        // try
        // {
        //     auto test_future = enqueue_task(ThreadType::IO, []()
        //     {
        //         LOG_DEBUG("ThreadManager self-test task executed");
        //         std::this_thread::sleep_for(std::chrono::milliseconds(10));
        //     });
        //
        //     if (test_future.wait_for(std::chrono::seconds(1)) == std::future_status::ready) {
        //         test_future.get();
        //         LOG_INFO("ThreadManager self-test PASSED");
        //     } else {
        //         LOG_WARNING("ThreadManager self-test FAILED - task didn't complete");
        //     }
        // } catch (const std::exception& e) {
        //     LOG_ERROR("ThreadManager self-test EXCEPTION: {}", e.what());
        // }
    }


    ThreadManager::~ThreadManager()
    {
        LOG_DEBUG("ThreadManager state - running: {}, active workers: {}",
                  m_running.load(), m_active_worker_count.load());
        stop_all(std::chrono::seconds(5), true);
        LOG_DEBUG("ThreadManager destructor exiting");
    }

    void ThreadManager::initialize_workers()
    {
        unsigned int hw_concurrency = std::thread::hardware_concurrency();
        if (hw_concurrency == 0) hw_concurrency = 2;

        const auto hardware_threads = static_cast<size_t>(hw_concurrency);
        size_t num_workers = std::max(m_config.min_workers, hardware_threads / 2);

        if (num_workers > m_config.max_workers)
        {
            num_workers = m_config.max_workers;
        }

        m_workers.reserve(num_workers);

        // Initializing worker contexts manually (cannot resize with atomic)
        m_worker_contexts.clear();
        for (size_t i = 0; i < num_workers; ++i)
        {
            m_worker_contexts.emplace_back(i); // Using emplace_back to construct in place
        }

        for (size_t i = 0; i < num_workers; ++i)
        {
            std::string worker_name = "Worker-" + std::to_string(i);

            m_workers.emplace_back(std::make_shared<ThreadInfo>(
                ThreadType::BACKGROUND,
                std::thread(&ThreadManager::worker_loop, this, i),
                false,
                worker_name
            ));

            ++m_active_worker_count;
        }

        LOG_DEBUG("Initialized {} worker threads", num_workers);
    }

    void ThreadManager::worker_loop(size_t worker_id)
    {
// Setting thread name for debugging
#ifdef __linux__
    std::string thread_name = "TM-Worker-" + std::to_string(worker_id);
    pthread_setname_np(pthread_self(), thread_name.c_str());
#endif

        LOG_DEBUG("Worker {} starting", worker_id);

        // Getting worker context
        WorkerContext* context = nullptr;
        if (worker_id < m_worker_contexts.size())
        {
            context = &m_worker_contexts[worker_id];
        } else {
            LOG_ERROR("Worker {} has no context!", worker_id);
            --m_active_worker_count;
            return;
        }

        // Getting worker info
        std::shared_ptr<ThreadInfo> worker_info;
        if (worker_id < m_workers.size())
        {
            worker_info = m_workers[worker_id];
        } else {
            LOG_ERROR("Worker {} has no ThreadInfo!", worker_id);
            --m_active_worker_count;
            return;
        }

        try
        {
            while (true)
            {
                // Checking if we should exit
                if (!m_running.load(std::memory_order_acquire) ||
                    m_emergency_stop.load(std::memory_order_acquire) ||
                    m_shutdown_requested.load(std::memory_order_acquire))
                {
                    break;
                }

                Task task;

                {
                    std::unique_lock<std::shared_mutex> lock(m_queue_mutex);

                    // Waiting for task or shutdown with timeout
                    bool has_task = m_queue_cv.wait_for(lock, std::chrono::milliseconds(100), [this]()
                    {
                        return m_total_queued_tasks.load(std::memory_order_acquire) > 0 ||
                               !m_running.load(std::memory_order_acquire) ||
                               m_emergency_stop.load(std::memory_order_acquire) ||
                               m_shutdown_requested.load(std::memory_order_acquire);
                    });

                    // Checking exit conditions
                    if (!has_task)
                    {
                        continue; // Timeout, check conditions again
                    }

                    if (!m_running.load(std::memory_order_acquire) ||
                        m_emergency_stop.load(std::memory_order_acquire) ||
                        m_shutdown_requested.load(std::memory_order_acquire))
                    {
                        break;
                    }

                    // If we woke up but there are no tasks, continue waiting
                    if (m_total_queued_tasks.load(std::memory_order_acquire) == 0)
                    {
                        continue;
                    }

                    // Getting next task
                    task = get_next_task_unsafe();
                }

                if (!task.func)
                {
                    // Got empty task, continue waiting
                    continue;
                }

                // Processing the task
                process_task(std::move(task), worker_id, context, worker_info);
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in worker loop {}: {}", worker_id, e.what());
        }

        LOG_DEBUG("Worker {} exiting", worker_id);
        m_active_worker_count.fetch_sub(1, std::memory_order_release);
    }

    ThreadManager::Task ThreadManager::get_next_task_unsafe()
    {
        // IMPORTANT: This method assumes m_queue_mutex is already locked!

        if (m_total_queued_tasks.load() == 0)
        {
            return Task{};
        }

        // 1. Checking priority queue first
        if (!m_priority_queue.empty())
        {
            Task task = std::move(const_cast<Task&>(m_priority_queue.top()));
            m_priority_queue.pop();
            --m_total_queued_tasks;
            return task;
        }

        // 2. Checking MAIN queue
        if (!m_task_queues[ThreadType::MAIN].empty())
        {
            Task task = std::move(m_task_queues[ThreadType::MAIN].front());
            m_task_queues[ThreadType::MAIN].pop();
            --m_total_queued_tasks;
            return task;
        }

        // 3. Checking SIMULATION queue
        if (!m_task_queues[ThreadType::SIMULATION].empty())
        {
            Task task = std::move(m_task_queues[ThreadType::SIMULATION].front());
            m_task_queues[ThreadType::SIMULATION].pop();
            --m_total_queued_tasks;
            return task;
        }

        // 4. Checking IO queue
        if (!m_task_queues[ThreadType::IO].empty())
        {
            Task task = std::move(m_task_queues[ThreadType::IO].front());
            m_task_queues[ThreadType::IO].pop();
            --m_total_queued_tasks;
            return task;
        }

        // 5. Checking BACKGROUND queue
        if (!m_task_queues[ThreadType::BACKGROUND].empty())
        {
            Task task = std::move(m_task_queues[ThreadType::BACKGROUND].front());
            m_task_queues[ThreadType::BACKGROUND].pop();
            --m_total_queued_tasks;
            return task;
        }

        return Task{};
    }

    ThreadManager::Task ThreadManager::get_next_task()
    {
        std::unique_lock<std::shared_mutex> lock(m_queue_mutex);

        if (m_total_queued_tasks.load() == 0)
        {
            return Task{};
        }

        // 1. Checking priority queue first
        if (!m_priority_queue.empty())
        {
            Task task = std::move(const_cast<Task&>(m_priority_queue.top()));
            m_priority_queue.pop();
            --m_total_queued_tasks;
            return task;
        }

        // 2. Checking MAIN queue
        if (!m_task_queues[ThreadType::MAIN].empty())
        {
            Task task = std::move(m_task_queues[ThreadType::MAIN].front());
            m_task_queues[ThreadType::MAIN].pop();
            --m_total_queued_tasks;
            return task;
        }

        // 3. Checking SIMULATION queue
        if (!m_task_queues[ThreadType::SIMULATION].empty())
        {
            Task task = std::move(m_task_queues[ThreadType::SIMULATION].front());
            m_task_queues[ThreadType::SIMULATION].pop();
            --m_total_queued_tasks;
            return task;
        }

        // 4. Checking IO queue
        if (!m_task_queues[ThreadType::IO].empty())
        {
            Task task = std::move(m_task_queues[ThreadType::IO].front());
            m_task_queues[ThreadType::IO].pop();
            --m_total_queued_tasks;
            return task;
        }

        // 5. Checking BACKGROUND queue
        if (!m_task_queues[ThreadType::BACKGROUND].empty())
        {
            Task task = std::move(m_task_queues[ThreadType::BACKGROUND].front());
            m_task_queues[ThreadType::BACKGROUND].pop();
            --m_total_queued_tasks;
            return task;
        }

        return Task{};
    }

    void ThreadManager::process_task(
        const Task& task,
        size_t worker_id,
        WorkerContext* context,
        const std::shared_ptr<ThreadInfo>& worker_info)
    {
        // Updating worker context
        context->busy.store(true);
        context->current_task_type = task.type;
        context->last_task_start = std::chrono::steady_clock::now();

        // Checking timeout
        const auto now = std::chrono::steady_clock::now();
        const auto wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - task.enqueue_time);

        if (task.timeout.count() > 0 && wait_time > task.timeout) {
            LOG_WARNING("Task for thread type {} timed out after {}ms (waited {}ms)",
                       thread_type_to_string(task.type),
                       task.timeout.count(),
                       wait_time.count());

            std::lock_guard<std::mutex> lock(m_stats_mutex);
            m_stats.tasks_timeout++;

            context->busy.store(false);
            return;
        }

        // Executing task
        auto start_time = std::chrono::steady_clock::now();
        bool success = false;

        try {
            // LOG_TRACE("Worker {} executing {} task",
            //          worker_id, thread_type_to_string(task.type));

            task.func();
            success = true;

            // LOG_TRACE("Worker {} completed {} task",
            //          worker_id, thread_type_to_string(task.type));
        }
        catch (const std::exception& e) {
            LOG_ERROR("Exception in worker {} processing {} task: {}",
                     worker_id, thread_type_to_string(task.type), e.what());
        }
        catch (...) {
            LOG_ERROR("Unknown exception in worker {} processing {} task",
                     worker_id, thread_type_to_string(task.type));
        }

        const auto end_time = std::chrono::steady_clock::now();
        const auto execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);

        // Updating statistics
        update_statistics(task, execution_time, success);

        // Updating worker info
        ++worker_info->tasks_processed;
        context->busy.store(false);

        // Logging long-running tasks
        if (execution_time > std::chrono::seconds(1)) {
            LOG_WARNING("Long-running task on thread type {}: {}ms (worker {})",
                       thread_type_to_string(task.type),
                       execution_time.count(),
                       worker_id);
        }
    }


    void ThreadManager::update_statistics(const Task& task,
                                         std::chrono::milliseconds execution_time,
                                         bool success)
    {
        std::lock_guard<std::mutex> lock(m_stats_mutex);

        m_stats.tasks_completed++;
        if (!success)
        {
            m_stats.tasks_failed++;
        }

        // Updating average execution time
        if (m_stats.tasks_completed > 0)
        {
            const size_t prev_total = static_cast<size_t>(m_stats.avg_task_time.count()) * (m_stats.tasks_completed - 1);
            const size_t new_avg = (prev_total + static_cast<size_t>(execution_time.count())) / m_stats.tasks_completed;
            m_stats.avg_task_time = std::chrono::milliseconds(new_avg);
        }

        // Updating max execution time
        if (execution_time > m_stats.max_task_time)
        {
            m_stats.max_task_time = execution_time;
        }
    }


    void ThreadManager::start_dedicated_thread(ThreadType thread_type,
                                          const std::string& name,
                                          const std::function<void()>& func,
                                          bool auto_restart)
    {
        if (!m_running.load())
        {
            throw std::runtime_error("ThreadManager is not running");
        }

        std::lock_guard<std::mutex> lock(m_thread_mutex);

        // Checking if thread with same name already exists
        for (const auto& thread : m_dedicated_threads)
        {
            if (thread->name == name && thread->thread.joinable())
            {
                LOG_WARNING("Dedicated thread '{}' already exists", name);
                return;
            }
        }

        // Creating thread_info first
        auto thread_info = std::make_shared<ThreadInfo>(
            ThreadType::BACKGROUND, // Temporary type, will be set correctly
            std::thread{},
            true,
            name,
            func
        );
        thread_info->type = thread_type;

        // Creating thread with the thread_info
        thread_info->thread = std::thread([this, thread_info, name, thread_type]()
        {
            dedicated_thread_loop(thread_info, thread_info->work_func, name, thread_type);
        });

        m_dedicated_threads.push_back(thread_info);

        LOG_INFO("Started dedicated thread '{}' of type {}",
                name, thread_type_to_string(thread_type));
    }

    void ThreadManager::dedicated_thread_loop(
        const std::shared_ptr<ThreadInfo>& thread_info,
        const std::function<void()>& func,
        const std::string& name,
        ThreadType thread_type)
    {
        LOG_DEBUG("Dedicated thread '{}' starting", name);

        try {
            while (thread_info->running.load(std::memory_order_acquire) &&
                   m_running.load(std::memory_order_acquire) &&
                   !m_emergency_stop.load(std::memory_order_acquire))
            {
                try
                {
                    func();
                } catch (const std::exception& e) {
                    LOG_ERROR("Exception in dedicated thread '{}': {}", name, e.what());

                    if (thread_type == ThreadType::MAIN)
                    {
                        emergency_stop("Main thread '" + name + "' crashed");
                        break;
                    }

                    // Small delay before continuing
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                // Small sleep in order to prevent tight loop
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        } catch (...) {
            LOG_ERROR("Unknown exception in dedicated thread loop '{}'", name);
        }

        LOG_DEBUG("Dedicated thread '{}' exiting", name);
    }

    void ThreadManager::stop_all(std::chrono::milliseconds wait_ms, bool force_shutdown)
{
    if (!m_running.load()) {
        return;
    }

    LOG_INFO("Stopping ThreadManager (wait: {}ms, force: {})...", wait_ms.count(), force_shutdown);

    // First stop accepting new tasks
    m_shutdown_requested.store(true, std::memory_order_release);

    // Signaling all threads to stop
    m_running.store(false, std::memory_order_release);

    // Notifying all waiting threads
    {
        std::unique_lock<std::shared_mutex> lock(m_queue_mutex);
        m_queue_cv.notify_all();
    }

    // Waiting for workers to finish current tasks
    const auto start_wait = std::chrono::steady_clock::now();
    while (m_active_worker_count.load() > 0 &&
           std::chrono::steady_clock::now() - start_wait < wait_ms)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Stopping performance monitor
    if (m_monitor_running.load())
    {
        m_monitor_running.store(false);
        if (m_monitor_thread.joinable())
        {
            m_monitor_thread.join();
        }
    }

    // Stopping dedicated threads
    {
        std::lock_guard<std::mutex> lock(m_thread_mutex);
        for (auto& thread_info : m_dedicated_threads)
        {
            thread_info->running.store(false, std::memory_order_release);
        }
    }

    // Force stop if needed
    shutdown_workers(force_shutdown);

    // Joining dedicated threads
    {
        std::lock_guard<std::mutex> lock(m_thread_mutex);
        for (auto& thread_info : m_dedicated_threads)
        {
            if (thread_info->thread.joinable())
            {
                if (force_shutdown)
                {
                    thread_info->thread.detach();
                } else {
                    thread_info->thread.join();
                }
            }
        }
        m_dedicated_threads.clear();
    }

    // Clearing all queues
    {
        std::unique_lock<std::shared_mutex> lock(m_queue_mutex);
        while (!m_priority_queue.empty())
        {
            m_priority_queue.pop();
        }
        for (auto& [type, queue] : m_task_queues)
        {
            std::queue<Task> empty;
            std::swap(queue, empty);
        }
        m_total_queued_tasks.store(0, std::memory_order_release);
    }

    LOG_INFO("ThreadManager stopped. Final stats: {} tasks completed, {} failed",
            m_stats.tasks_completed, m_stats.tasks_failed);
}

    void ThreadManager::shutdown_workers(bool force)
    {
        LOG_DEBUG("Shutting down workers (force: {})", force);

        // Signaling all workers to stop
        for (auto& worker_info : m_workers)
        {
            worker_info->running.store(false, std::memory_order_release);
        }

        // Notifying all workers
        {
            std::unique_lock<std::shared_mutex> lock(m_queue_mutex);
            m_queue_cv.notify_all();
        }

        // Joining worker threads with timeout
        for (auto& worker_info : m_workers)
        {
            if (worker_info->thread.joinable())
            {
                if (force)
                {
                    worker_info->thread.detach();
                } else {
                    // Trying to join with timeout
                    auto start = std::chrono::steady_clock::now();
                    while (worker_info->thread.joinable() &&
                           std::chrono::steady_clock::now() - start < std::chrono::seconds(5))
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    }

                    if (worker_info->thread.joinable())
                    {
                        LOG_WARNING("Worker thread didn't stop in time, detaching");
                        worker_info->thread.detach();
                    } else {
                        worker_info->thread.join();
                    }
                }
            }
        }

        m_workers.clear();
        m_active_worker_count.store(0, std::memory_order_release);
    }

    void ThreadManager::emergency_stop(const std::string& reason)
    {
        LOG_ERROR("EMERGENCY STOP: {}", reason);
        m_emergency_stop.store(true);
        stop_all(std::chrono::milliseconds(100), true);
    }

    size_t ThreadManager::get_queue_size(ThreadType thread_type) const
    {
        std::shared_lock<std::shared_mutex> lock(m_queue_mutex);

        if (thread_type == ThreadType::MAIN || thread_type == ThreadType::SIMULATION)
        {
            // Counting tasks in priority queue
            size_t count = 0;
            auto temp_queue = m_priority_queue;
            while (!temp_queue.empty())
            {
                if (temp_queue.top().type == thread_type)
                {
                    count++;
                }
                temp_queue.pop();
            }
            return count;
        } else {
            const auto it = m_task_queues.find(thread_type);
            if (it != m_task_queues.end()) {
                return it->second.size();
            }
            return 0;
        }
    }

    size_t ThreadManager::get_dedicated_threads() const
    {
        std::lock_guard<std::mutex> lock(m_thread_mutex);
        return m_dedicated_threads.size();
    }

    ThreadManager::Statistics ThreadManager::get_statistics() const
    {
        std::lock_guard<std::mutex> lock(m_stats_mutex);
        return m_stats;
    }

    void ThreadManager::set_config(const Config& config)
    {
        C2L_ASSERT(config.min_workers > 0, "Minimum workers must be > 0");
        C2L_ASSERT(config.max_workers >= config.min_workers,
                   "Max workers must be >= min workers");

        std::lock_guard<std::mutex> lock(m_thread_mutex);
        m_config = config;

        LOG_INFO("ThreadManager configuration updated");
    }

    bool ThreadManager::stop_dedicated_thread(const std::string& name,
                                         std::chrono::milliseconds wait_ms)
    {
        std::unique_lock<std::mutex> lock(m_thread_mutex);

        auto it = std::find_if(m_dedicated_threads.begin(), m_dedicated_threads.end(),
            [&name](const std::shared_ptr<ThreadInfo>& thread)
            {
                return thread->name == name;
            });

        if (it == m_dedicated_threads.end())
        {
            LOG_WARNING("Dedicated thread '{}' not found", name);
            return false;
        }

        // Signaling thread to stop
        (*it)->running.store(false);

        lock.unlock(); // Unlock before waiting

        // Waiting for thread to finish
        if ((*it)->thread.joinable()) {
            auto start = std::chrono::steady_clock::now();

            while ((*it)->thread.joinable())
            {
                auto elapsed = std::chrono::steady_clock::now() - start;

                if (wait_ms.count() > 0 && elapsed > wait_ms)
                {
                    LOG_WARNING("Dedicated thread '{}' didn't stop in time, detaching", name);
                    (*it)->thread.detach();

                    std::lock_guard<std::mutex> lock_again(m_thread_mutex);
                    m_dedicated_threads.erase(it);
                    return false;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        std::lock_guard<std::mutex> lock_again(m_thread_mutex);
        m_dedicated_threads.erase(it);

        LOG_INFO("Stopped dedicated thread '{}'", name);
        return true;
    }

    void ThreadManager::monitor_performance()
    {
        while (m_monitor_running.load() && should_continue())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (!m_monitor_running.load())
            {
                break;
            }

            // Collecting performance metrics
            size_t busy_workers = 0;
            for (const auto& context : m_worker_contexts)
            {
                if (context.busy.load())
                {
                    busy_workers++;
                }
            }

            size_t current_workers = m_workers.size();
            float utilization = current_workers > 0 ?
                static_cast<float>(busy_workers) / static_cast<float>(current_workers) * 100.0f : 0.0f;

            // Logging performance info periodically
            static size_t log_counter = 0;
            if (++log_counter % 12 == 0) // Every minute
            {
                LOG_INFO("ThreadManager Performance - Workers: {}/{}, Utilization: {}, Queue: {}",
                        busy_workers, current_workers, utilization,
                        m_total_queued_tasks.load());
            }
        }

        LOG_DEBUG("Performance monitor shutting down");
    }

    bool ThreadManager::should_continue() const noexcept
    {
        return m_running.load(std::memory_order_acquire) &&
               !m_emergency_stop.load(std::memory_order_acquire) &&
               !m_shutdown_requested.load(std::memory_order_acquire) &&
               !m_paused.load(std::memory_order_acquire);
    }

    std::vector<ThreadManager::ThreadType> ThreadManager::get_available_thread_types()
    {
        return {
            ThreadType::MAIN,
            ThreadType::SIMULATION,
            ThreadType::IO,
            ThreadType::BACKGROUND
        };
    }

    size_t ThreadManager::get_active_workers() const
    {
        return m_active_worker_count.load(std::memory_order_acquire);
    }

    size_t ThreadManager::get_total_queue_size() const
    {
        return m_total_queued_tasks.load(std::memory_order_acquire);
    }

    bool ThreadManager::is_running() const noexcept
    {
        return m_running.load(std::memory_order_acquire);
    }

    bool ThreadManager::is_emergency_stop() const noexcept
    {
        return m_emergency_stop.load(std::memory_order_acquire);
    }

    ThreadManager::Config ThreadManager::get_config() const
    {
        std::lock_guard<std::mutex> lock(m_thread_mutex);
        return m_config;
    }

    void ThreadManager::cleanup_finished_threads()
    {
        std::lock_guard<std::mutex> lock(m_thread_mutex);

        // Removing finished dedicated threads
        auto it = m_dedicated_threads.begin();
        while (it != m_dedicated_threads.end())
        {
            if ((*it)->thread.joinable() && !(*it)->running.load())
            {
                (*it)->thread.join();
                it = m_dedicated_threads.erase(it);
            } else {
                ++it;
            }
        }
    }

    void ThreadManager::adjust_thread_pool()
    {
        if (!m_config.enable_dynamic_scaling) return;

        std::lock_guard<std::mutex> lock(m_thread_mutex);

        size_t current_workers = m_workers.size();
        size_t busy_workers = 0;

        for (const auto& context : m_worker_contexts)
        {
            if (context.busy.load()) busy_workers++;
        }

        float utilization = current_workers > 0 ?
            static_cast<float>(busy_workers) / static_cast<float>(current_workers) * 100.0f : 0.0f;

        // Scaling logic
        size_t target_workers = current_workers;

        if (utilization > 80.0f && current_workers < m_config.max_workers)
        {
            // Scaling up
            target_workers = std::min(current_workers * 2, m_config.max_workers);
        } else if (utilization < 20.0f && current_workers > m_config.min_workers) {
            // Scale down
            target_workers = std::max(current_workers / 2, m_config.min_workers);
        }

        if (target_workers != current_workers)
        {
            scale_thread_pool(target_workers);
        }
    }

    void ThreadManager::scale_thread_pool(size_t target_workers)
    {
        std::lock_guard<std::mutex> lock(m_thread_mutex);

        if (target_workers == m_workers.size()) return;

        LOG_INFO("Scaling thread pool from {} to {} workers",
                 m_workers.size(), target_workers);

        if (target_workers > m_workers.size())
        {
            // Add new workers
            size_t start_id = m_workers.size();
            for (size_t i = start_id; i < target_workers; ++i)
            {
                m_worker_contexts.emplace_back(i);

                std::string worker_name = "Worker-" + std::to_string(i);
                m_workers.emplace_back(std::make_shared<ThreadInfo>(
                    ThreadType::BACKGROUND,
                    std::thread(&ThreadManager::worker_loop, this, i),
                    false,
                    worker_name
                ));

                ++m_active_worker_count;
            }
        } else {
            // Removing excess workers
            size_t workers_to_remove = m_workers.size() - target_workers;

            for (size_t i = 0; i < workers_to_remove; ++i)
            {
                if (!m_workers.back()->running.load())
                {
                    if (m_workers.back()->thread.joinable())
                    {
                        m_workers.back()->thread.join();
                    }
                    m_workers.pop_back();
                    --m_active_worker_count;
                }
            }
        }
    }

    bool ThreadManager::can_steal_work(ThreadType from, ThreadType to) const
    {
        if (!m_config.enable_work_stealing) return false;

        // Higher priority threads can steal from lower priority ones
        const auto from_val = static_cast<uint8_t>(from);
        const auto to_val = static_cast<uint8_t>(to);

        // Lower numeric value = higher priority (MAIN = 0, BACKGROUND = 3)
        return to_val < from_val; // Higher priority can steal from lower priority
    }


} // namespace c2l::core