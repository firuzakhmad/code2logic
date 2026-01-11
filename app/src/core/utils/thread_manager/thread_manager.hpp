#pragma once

#include "core/utils/logger/logger.hpp"

#include <vector>
#include <thread>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <future>
#include <memory>
#include <unordered_map>
#include <chrono>
#include <shared_mutex>

namespace c2l::core
{

    /**
     * @class ThreadManager
     * @brief Professional thread orchestration system with priority-based scheduling
     *
     * Key Features:
     * - Priority-based thread scheduling with thread type support
     * - Multiple priority queues (MAIN > SIMULATION > IO > BACKGROUND)
     * - Work-stealing task queue
     * - Sub-millisecond task dispatch latency
     * - Exception resilience policies with retry mechanisms
     * - Thread affinity support
     * - Performance monitoring
     * - Dynamic thread scaling
     *
     * @invariant All public methods are thread-safe
     */
    class ThreadManager final
    {
    public:
        /**
         * @enum ThreadType
         * @brief Categorizes threads by priority/function
         */
        enum class ThreadType : uint8_t
        {
            MAIN,       // Highest priority - UI, rendering, input
            SIMULATION, // Medium priority - game logic, physics
            IO,         // Lowest priority - file I/O, network, loading
            BACKGROUND  // Very low priority - cleanup, analytics
        };

        /**
         * @struct Config
         * @brief Configuration for ThreadManager
         */
        struct Config
        {
            // Using explicit constructor for safety
            explicit Config()
                : min_workers{2}
            , max_workers{std::thread::hardware_concurrency()}
            , max_queue_size{1000}
            , task_timeout{5000}
            , enable_work_stealing{true}
            , enable_performance_monitoring{true}
            , enable_dynamic_scaling{true}
            , thread_configs({
                {ThreadType::MAIN, {1, 2, std::chrono::milliseconds(100), false}},
                {ThreadType::SIMULATION, {1, 4, std::chrono::milliseconds(500), true}},
                {ThreadType::IO, {1, 8, std::chrono::milliseconds(10000), true}},
                {ThreadType::BACKGROUND, {1, 2, std::chrono::milliseconds(30000), true}}})
            {}

            size_t min_workers;
            size_t max_workers;
            size_t max_queue_size;
            std::chrono::milliseconds task_timeout;
            bool enable_work_stealing;
            bool enable_performance_monitoring;
            bool enable_dynamic_scaling;

            /**
             * @struct ThreadTypeConfig
             * @brief Configuration for specific thread types
             */
            struct ThreadTypeConfig
            {
                size_t min_threads                  {1};
                size_t max_threads                  {4};
                std::chrono::milliseconds timeout   {10000};
                bool allow_work_stealing            {true};
            };

            std::unordered_map<ThreadType, ThreadTypeConfig> thread_configs;
        };

        /**
         * @struct Statistics
         * @brief ThreadManager performance statistics
         */
        struct Statistics
        {
            size_t tasks_completed                      {0};
            size_t tasks_failed                         {0};
            size_t tasks_timeout                        {0};
            size_t total_queue_size                     {0};
            size_t active_workers                       {0};
            std::chrono::milliseconds avg_task_time     {0};
            std::chrono::milliseconds max_task_time     {0};
            std::chrono::milliseconds total_runtime     {0};

            // Per thread type statistics
            std::unordered_map<ThreadType, size_t> tasks_by_type;
            std::unordered_map<ThreadType, std::chrono::milliseconds> avg_time_by_type;

            void reset() { *this = Statistics(); }
        };

        explicit ThreadManager(Config config = Config{});
        ~ThreadManager();

        // Non-copyable, non-movable
        ThreadManager(const ThreadManager&) = delete;
        ThreadManager& operator=(const ThreadManager&) = delete;
        ThreadManager(ThreadManager&&) = delete;
        ThreadManager& operator=(ThreadManager&&) = delete;

        // Task Management
        /**
         * @brief Enqueues a task with thread type specification
         * @tparam F Callable type
         * @tparam Args Argument types
         * @param thread_type Priority/type of thread to execute on
         * @param f Callable to execute
         * @param args Arguments to forward to callable
         * @return std::future containing task result
         *
         * @throws std::runtime_error if queue is full or manager is stopped
         *
         * @complexity Amortized O(1)
         */
        template<typename F, typename... Args>
        auto enqueue_task(ThreadType thread_type, F&& f, Args&&... args)
            -> std::future<std::invoke_result_t<F, Args...>>;

        /**
         * @brief Enqueues a task with timeout
         * @param thread_type Priority/type of thread to execute on
         * @param timeout_ms Timeout in milliseconds (0 = no timeout)
         * @param f Function to execute
         * @param args Function arguments to pass
         * @return std::future containing task result
         */
        template<typename F, typename... Args>
        auto enqueue_task_with_timeout(ThreadType thread_type,
                                       std::chrono::milliseconds timeout_ms,
                                       F&& f, Args&&... args)
            -> std::future<std::invoke_result_t<F, Args...>>;

        /**
         * @brief Enqueues a batch of tasks
         * @param thread_type Priority/type of thread to execute on
         * @param tasks Vector of tasks to execute
         * @return Vector of futures for each task
         */
        template<typename F>
        std::vector<std::future<void>> enqueue_task_batch(
            ThreadType thread_type,
            const std::vector<F>& tasks);

        // Thread Management
        /**
         * @brief Starts a dedicated thread for specific work
         * @param thread_type Thread priority classification
         * @param name Thread name for debugging
         * @param func Callable to execute
         * @param auto_restart Restart thread if it crashes
         */
        void start_dedicated_thread(ThreadType thread_type,
                                    const std::string& name,
                                    const std::function<void()>& func,
                                    bool auto_restart = false);

        /**
         * @brief Stops a dedicated thread
         * @param name Thread name to stop
         * @param wait_ms Maximum wait time in milliseconds
         * @return true if thread stopped successfully
         */
        bool stop_dedicated_thread(const std::string& name,
                                  std::chrono::milliseconds wait_ms = std::chrono::seconds(5));

        /**
         * @brief Scales thread pool based on workload
         * @param target_workers Target number of workers
         */
        void scale_thread_pool(size_t target_workers);

        // Control Methods
        /**
         * @brief Graceful shutdown
         * @param wait_ms Maximum wait time for tasks to complete
         * @param force_shutdown Force shutdown if timeout occurs
         */
        void stop_all(std::chrono::milliseconds wait_ms = std::chrono::seconds(10),
                      bool force_shutdown = true);

        /**
         * @brief Emergency stop - immediate termination
         * @param reason Reason for emergency stop
         */
        void emergency_stop(const std::string& reason = "Emergency stop requested");

        /**
         * @brief Check if manager is running
         * @return true if manager is running
         */
        bool is_running() const noexcept;

        /**
         * @brief Check if emergency stop was requested
         * @return true if emergency stop was requested
         */
        bool is_emergency_stop() const noexcept;

        /**
         * @brief Thread continuation predicate
         * @return false if emergency_stop() was called
         */
        bool should_continue() const noexcept;

        // Query Methods
        /**
         * @brief Get current queue size for a thread type
         * @param thread_type Thread type to query
         * @return Number of pending tasks
         */
        size_t get_queue_size(ThreadType thread_type) const;

        /**
         * @brief Get total queue size across all thread types
         * @return Total number of pending tasks
         */
        size_t get_total_queue_size() const;

        /**
         * @brief Get number of active worker threads
         * @return Number of active workers
         */
        size_t get_active_workers() const;

        /**
         * @brief Get number of dedicated threads
         * @return Number of dedicated threads
         */
        size_t get_dedicated_threads() const;

        /**
         * @brief Get performance statistics
         * @return Current statistics
         */
        Statistics get_statistics() const;

        /**
         * @brief Get configuration
         * @return Current configuration
         */
        Config get_config() const;

        /**
         * @brief Update configuration
         * @param config New configuration
         */
        void set_config(const Config& config);

        /**
         * @brief Get available thread types
         * @return Vector of available thread types
         */
        static std::vector<ThreadType> get_available_thread_types();

        /**
         * @brief Convert thread type to string
         * @param type Thread type
         * @return const char* representation
         */
        static const char* thread_type_to_string(ThreadType type);

    private:
        struct Task
        {
            std::function<void()> func;
            ThreadType type;
            std::chrono::steady_clock::time_point enqueue_time;
            std::chrono::milliseconds timeout{0};
            std::string description;

            // For priority queue ordering (lower type value = higher priority)
            bool operator<(const Task& other) const noexcept {
                return static_cast<uint8_t>(type) > static_cast<uint8_t>(other.type);
            }
        };

        struct ThreadInfo
        {
            ThreadType type;
            std::thread thread;
            std::atomic<bool> running{false};
            std::atomic<bool> dedicated{false};
            std::string name;
            std::function<void()> work_func;
            std::atomic<size_t> tasks_processed{0};
            std::chrono::steady_clock::time_point start_time;

            ThreadInfo(ThreadType t, std::thread&& thr, bool ded,
                      std::string n = "", std::function<void()> wf = nullptr)
                : type(t), thread(std::move(thr)), dedicated(ded),
                  name(std::move(n)), work_func(std::move(wf)),
                  start_time(std::chrono::steady_clock::now())
            {
                running.store(true);
            }

            // Non-copyable
            ThreadInfo(const ThreadInfo&) = delete;
            ThreadInfo& operator=(const ThreadInfo&) = delete;

            // Movable
            ThreadInfo(ThreadInfo&& other) noexcept
                : type(other.type),
                  thread(std::move(other.thread)),
                  running(other.running.load()),
                  dedicated(other.dedicated.load()),
                  name(std::move(other.name)),
                  work_func(std::move(other.work_func)),
                  tasks_processed(other.tasks_processed.load()),
                  start_time(other.start_time) {}
        };

        struct WorkerContext
        {
            size_t id;
            std::atomic<bool> busy;
            std::chrono::steady_clock::time_point last_task_start;
            ThreadType current_task_type;

            // Default constructor
            WorkerContext()
                : id(0)
                , busy(false)
                , last_task_start(std::chrono::steady_clock::now())
                , current_task_type(ThreadType::BACKGROUND)
            {}

            // Parameterized constructor
            explicit WorkerContext(size_t worker_id)
                : id(worker_id)
                , busy(false)
                , last_task_start(std::chrono::steady_clock::now())
                , current_task_type(ThreadType::BACKGROUND)
            {}

            // Delete copy operations (atomic is not copyable)
            WorkerContext(const WorkerContext&) = delete;
            WorkerContext& operator=(const WorkerContext&) = delete;

            // Allow move (but be careful with atomic)
            WorkerContext(WorkerContext&& other) noexcept
                : id(other.id)
                , busy(other.busy.load())
                , last_task_start(other.last_task_start)
                , current_task_type(other.current_task_type)
            {
                // Atomic load during move
            }

            WorkerContext& operator=(WorkerContext&& other) noexcept
            {
                if (this != &other) {
                    id = other.id;
                    busy.store(other.busy.load());
                    last_task_start = other.last_task_start;
                    current_task_type = other.current_task_type;
                }
                return *this;
            }
        };

        // Private Methods
        void initialize_workers();
        void shutdown_workers(bool force = false);
        void worker_loop(size_t worker_id);

        /**
         * @note Non-safe -> This method assumes m_queue_mutex is already locked!
         * @return Returns Task
         */
        Task get_next_task_unsafe();

        /**
         * @note Thread-safe -> This method locks using m_queue_mutex!
         * @return Returns Task
         */
        Task get_next_task();

        void dedicated_thread_loop(
            const std::shared_ptr<ThreadInfo>& thread_info,
            const std::function<void()>& func,
            const std::string& name,
            ThreadType thread_type);

        void process_task(
            const Task& task,
            size_t worker_id,
            WorkerContext* context,
            const std::shared_ptr<ThreadInfo>& worker_info);
        void update_statistics(const Task& task,
                              std::chrono::milliseconds execution_time,
                              bool success);
        void cleanup_finished_threads();
        void monitor_performance();
        void adjust_thread_pool();
        bool can_steal_work(ThreadType from, ThreadType to) const;

        // -------------------------------------------------------------------------
        // Member Variables
        // -------------------------------------------------------------------------
        Config m_config;

        // Worker management
        std::vector<std::shared_ptr<ThreadInfo>> m_workers;
        std::vector<WorkerContext> m_worker_contexts;
        std::vector<std::shared_ptr<ThreadInfo>> m_dedicated_threads;

        // Task queues - one for each thread type
        std::unordered_map<ThreadType, std::queue<Task>> m_task_queues;
        std::priority_queue<Task> m_priority_queue; // For high-priority tasks

        // Synchronization
        mutable std::shared_mutex m_queue_mutex;
        std::condition_variable_any m_queue_cv;
        mutable std::mutex m_thread_mutex;
        std::condition_variable m_thread_cv;

        // State management
        std::atomic<bool> m_running{false};
        std::atomic<bool> m_paused{false};
        std::atomic<bool> m_emergency_stop{false};
        std::atomic<bool> m_shutdown_requested{false};
        std::atomic<size_t> m_total_queued_tasks{0};
        std::atomic<size_t> m_active_worker_count{0};

        // Statistics
        mutable std::mutex m_stats_mutex;
        Statistics m_stats;
        std::chrono::steady_clock::time_point m_start_time;

        // Performance monitoring thread
        std::thread m_monitor_thread;
        std::atomic<bool> m_monitor_running{false};
        std::condition_variable m_monitor_cv;
        mutable std::mutex m_monitor_mutex;
    };

    // Template Implementations
    template<typename F, typename... Args>
    auto ThreadManager::enqueue_task(ThreadType thread_type, F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        using return_type = std::invoke_result_t<F, Args...>;

        if (!m_running.load() || m_emergency_stop.load()) {
            throw std::runtime_error("ThreadManager is not running");
        }

        // Creating packaged task
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            [func = std::forward<F>(f), args_tuple = std::make_tuple(std::forward<Args>(args)...)]() mutable
            {
                return std::apply(func, std::move(args_tuple));
            }
        );

        std::future<return_type> result = task->get_future();

        {
            std::unique_lock<std::shared_mutex> lock(m_queue_mutex);

            // Checking queue limits
            const auto& config = m_config.thread_configs[thread_type];
            size_t queue_size = m_task_queues[thread_type].size();

            if (m_total_queued_tasks.load() >= m_config.max_queue_size) {
                LOG_ERROR("Task queue is full ({} tasks), rejecting new task",
                         m_total_queued_tasks.load());
                throw std::runtime_error("Task queue is full");
            }

            if (queue_size >= m_config.max_queue_size / 4) { // Per-type limit
                LOG_WARNING("Thread type {} queue is getting full ({} tasks)",
                           thread_type_to_string(thread_type), queue_size);
            }

            // Creating task
            Task t;
            t.func = [task]() { (*task)(); };
            t.type = thread_type;
            t.enqueue_time = std::chrono::steady_clock::now();
            t.timeout = config.timeout;
            t.description = typeid(F).name();

            // Enqueuing based on priority
            if (thread_type == ThreadType::MAIN || thread_type == ThreadType::SIMULATION) {
                m_priority_queue.push(std::move(t));
            } else {
                m_task_queues[thread_type].push(std::move(t));
            }

            ++m_total_queued_tasks;
            m_stats.total_queue_size = m_total_queued_tasks.load();

            // Updating per-type statistics
            {
                std::lock_guard<std::mutex> stats_lock(m_stats_mutex);
                m_stats.tasks_by_type[thread_type]++;
            }
        }

        m_queue_cv.notify_one();

        // LOG_TRACE("Enqueued task for thread type {} (total queued: {})",
        //          thread_type_to_string(thread_type), m_total_queued_tasks.load());

        return result;
    }

    template<typename F, typename... Args>
    auto ThreadManager::enqueue_task_with_timeout(ThreadType thread_type,
                                                 std::chrono::milliseconds timeout_ms,
                                                 F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        using return_type = std::invoke_result_t<F, Args...>;

        auto task_promise = std::make_shared<std::promise<return_type>>();
        std::future<return_type> result = task_promise->get_future();

        // Enqueuing the actual task
        auto future = enqueue_task(thread_type, std::forward<F>(f), std::forward<Args>(args)...);

        // Handling timeout_ms in a separate thread
        m_workers.emplace_back(std::make_shared<ThreadInfo>(
            ThreadType::BACKGROUND,
            std::thread([this, task_promise, future = std::move(future), timeout_ms]() mutable {
                auto status = future.wait_for(timeout_ms);

                if (status == std::future_status::ready) {
                    try {
                        task_promise->set_value(future.get());
                    } catch (...) {
                        task_promise->set_exception(std::current_exception());
                    }
                } else {
                    task_promise->set_exception(
                        std::make_exception_ptr(std::runtime_error("Task timeout_ms")));

                    std::lock_guard<std::mutex> lock(m_stats_mutex);
                    m_stats.tasks_timeout++;
                }
            }),
            false,
            "timeout_handler"
        ));

        return result;
    }

    template<typename F>
    std::vector<std::future<void>> ThreadManager::enqueue_task_batch(
        const ThreadType thread_type,
        const std::vector<F>& tasks)
    {
        std::vector<std::future<void>> futures;
        futures.reserve(tasks.size());

        {
            std::unique_lock<std::shared_mutex> lock(m_queue_mutex);

            for (const auto& task_func : tasks) {
                if (m_total_queued_tasks.load() >= m_config.max_queue_size) {
                    LOG_ERROR("Task queue full during batch enqueue");
                    break;
                }

                auto task = std::make_shared<std::packaged_task<void()>>(task_func);
                futures.emplace_back(task->get_future());

                Task t;
                t.func = [task]() { (*task)(); };
                t.type = thread_type;
                t.enqueue_time = std::chrono::steady_clock::now();
                t.timeout = m_config.thread_configs[thread_type].timeout;
                t.description = "batch_task";

                if (thread_type == ThreadType::MAIN || thread_type == ThreadType::SIMULATION) {
                    m_priority_queue.push(std::move(t));
                } else {
                    m_task_queues[thread_type].push(std::move(t));
                }

                ++m_total_queued_tasks;
            }

            m_stats.total_queue_size = m_total_queued_tasks.load();
        }

        if (!tasks.empty()) {
            m_queue_cv.notify_all(); // Notifying all workers for batch processing
        }

        LOG_DEBUG("Enqueued batch of {} tasks for thread type {}",
                 futures.size(), thread_type_to_string(thread_type));

        return futures;
    }

} // namespace c2l::core