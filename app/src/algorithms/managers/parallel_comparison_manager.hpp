#ifndef CODE2LOGIC_PARALLEL_COMPARISON_MANAGER_HPP
#define CODE2LOGIC_PARALLEL_COMPARISON_MANAGER_HPP

#include "algorithms/core/i_simple_algorithm.hpp"
#include "algorithms/core/atomic_performance_metrics.hpp"
#include "algorithms/core/algorithm_observer.hpp"
#include "algorithms/core/i_algorithm_metadata.hpp"
#include "algorithms/visualizers/i_algorithm_visualizer.hpp"
#include "algorithms/core/algorithm_types.hpp"
#include "core/utils/thread_manager/thread_manager.hpp"
#include "core/json_config_manager/json_config_manager.hpp"
#include "algorithms/core/algorithm_step.hpp"
#include "algorithms/core/algorithm_registry.hpp"
#include "algorithms/managers/parallel_algorithm.hpp"

#include <memory>
#include <mutex>
#include <string_view>
#include <condition_variable>
#include <shared_mutex>

namespace c2l::algorithms
{
    class ParallelComparisonManager final : public AlgorithmObserver
    {
    public:
        explicit ParallelComparisonManager(
            core::ThreadManager& thread_manager,
            AlgorithmRegistry& algorithm_registry);

        ~ParallelComparisonManager() override;

        ParallelComparisonManager(const ParallelComparisonManager&) = delete;
        ParallelComparisonManager& operator=(const ParallelComparisonManager&) = delete;

        bool set_algorithm_left(AlgorithmType type);
        bool set_algorithm_right(AlgorithmType type);

        // Data management
        void set_shared_data(const std::vector<int>& data);
        void generate_random_data(size_t size = 15, int max_value = 200);

        // Parallel playback control
        void play();
        void pause();
        void stop();
        void step_forward(); 
        void step_backward();
        void reset();

        // Speed control
        void set_speed(float speed);
        float get_speed() const noexcept;

        // State queries
        bool is_playing() const noexcept;
        bool is_paused() const noexcept;
        bool is_complete() const;

        // Getters
        const ParallelAlgorithm& get_left_algorithm() const noexcept;
        const ParallelAlgorithm& get_right_algorithm() const noexcept;
        const std::vector<int>& get_current_data() const noexcept;

        // Observer interface
        void on_step_changed() override;
        void on_algorithm_completed() override;

        // Performance comparison with parallel metrics
        struct ParallelComparisonResult
        {
            struct AlgorithmResult
            {
                std::chrono::microseconds total_time    {0};
                size_t steps                            {0};
                size_t comparisons                      {0};
                size_t swaps                            {0};
                double speed_score                      {0};
                double efficiency_score                 {0.0};
                double avg_step_time_ms                 {0.0};
                size_t visited_node_count               {0};
                size_t explored_node_count              {0};
            };

            AlgorithmResult left        {};
            AlgorithmResult right       {};

            std::string_view winner;
            double performance_ratio    {0.0};
            double parallel_efficiency  {0.0};

            [[nodiscard]] bool has_winner() const noexcept
            {
                return !winner.empty();
            }
        };

        [[nodiscard]] ParallelComparisonResult
        get_parallel_comparison_result() const;

    private:
        bool initialize_algorithm(
            ParallelAlgorithm& algorithm,
            AlgorithmType type,
            size_t id
        );

        void update_metrics(
            ParallelAlgorithm& algorithm,
            const AlgorithmStep& step
        );

        // Parallel execution methods
        void execute_parallel_step(bool forward);
        void submit_step_task(
            ParallelAlgorithm& algorithm,
            bool forward
        );
        void wait_for_all_steps();

        // Background execution with ThreadManager 
        void start_background_playback();
        void stop_background_playback();
        void background_playback_loop();

        core::ThreadManager& m_thread_manager;
        AlgorithmRegistry& m_algorithm_registry;

        // The two algorithm being compared in parallel
        ParallelAlgorithm m_left;
        ParallelAlgorithm m_right;

        // Shared data
        std::vector<int> m_current_data;

        // Playback state
        std::atomic<bool> m_is_playing                  {false};
        std::atomic<bool> m_is_paused                   {false};
        std::atomic<float> m_speed                      {1.0f};
        std::atomic<bool> m_parallel_execution_active   {false};

        // ThreadManager tasks
        std::vector<std::future<void>> m_parallel_tasks;
        std::mutex m_tasks_mutex;

        // Synchronization
        mutable std::shared_mutex m_comparison_mutex;
        std::atomic<size_t> m_pending_steps             {0};
        std::condition_variable m_step_completion_cv;
        std::mutex m_step_mutex;

        //Timing for performance metrics
        std::chrono::high_resolution_clock::time_point m_playback_start_time;
        std::atomic<std::chrono::microseconds::rep> m_total_playback_time{0};
    };
    
} // namespace c2l::algorithms

#endif //CODE2LOGIC_PARALLEL_COMPARISON_MANAGER_HPP