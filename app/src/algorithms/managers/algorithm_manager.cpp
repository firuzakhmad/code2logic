//
// Created by Akhmad on 11/8/25.
//

#include "algorithm_manager.hpp"
#include "algorithms/bubble_sort.hpp"
#include "algorithms/quick_sort.hpp"
#include "algorithms/visualizers/array_based_visualizer.hpp"
#include "core/utils/logger/logger.hpp"

#include <utility>
#include <random>
#include <chrono>

#include <glm/glm.hpp>
#include "imgui.h"

namespace c2l::algorithms
{
    AlgorithmManager::AlgorithmManager(
        core::ThreadManager& thread_manager,
        AlgorithmRegistry& algorithm_registry)
        : m_thread_manager{thread_manager}
        , m_algorithm_registry{algorithm_registry}
    {}

    AlgorithmManager::~AlgorithmManager()
    {
        LOG_DEBUG("AlgorithmManager shutting down...");
        stop_background_execution();
        unload_current_algorithm();
        LOG_DEBUG("AlgorithmManager destroyed");
    }

    bool AlgorithmManager::load_algorithm(AlgorithmType type)
    {
        stop_background_execution();

        unload_current_algorithm();

        {
            std::unique_lock lock(m_algorithm_mutex);

            // Check if algorithm exists in registry
            if (!m_algorithm_registry.has_algorithm(type))
            {
                LOG_ERROR(
                    "Algorithm type '{}' not registered in registry",
                    algorithm_display_name(type)
                );
                return false;
            }

            // Creating new algorithm instance using registry
            auto new_context = AlgorithmContext{};
            new_context.execution = m_algorithm_registry.create_algorithm(type);
            new_context.visualizer = m_algorithm_registry.create_visualizer(type);

            if (!new_context.execution || !new_context.visualizer)
            {
                LOG_ERROR(
                    "Failed to create algorithm/visualizer instance for type: {}",
                    algorithm_display_name(type)
                );
                return false;
            }

            new_context.metadata = new_context.execution->metadata();
            new_context.type = type;
            new_context.name = algorithm_display_name(type);

            // Validate metadata
            if (!new_context.metadata)
            {
                LOG_ERROR("Algorithm does not provide metadata");
                return false;
            }

            if (!new_context.metadata->is_valid())
            {
                LOG_ERROR(
                    "Algorithm metadata is invalid for: {}",
                    new_context.name)
                ;
                return false;
            }

            // Attaching observer
            new_context.execution->add_observer(this);

            // Initializing visualizer
            if (new_context.visualizer)
            {
                new_context.visualizer->initialize(
                    new_context.execution.get(),
                    new_context.metadata
                );
            }

            // Initialize with random data
            try
            {
                auto default_data = generate_random_data();
                new_context.execution->initialize(default_data);
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("Failed to initialize algorithm: {}", e.what());
                return false;
            }

            // Move new context into current context
            m_current_context = std::move(new_context);

            LOG_INFO("Loaded algorithm: {} (v{})",
                m_current_context.name,
                m_current_context.metadata->get_complexity().is_valid() ? "JSON" : "Legacy"
            );
        }

        m_last_step_index = std::numeric_limits<size_t>::max();
        update_highlight_cache();

        return true;
    }

    bool AlgorithmManager::load_algorithm(const std::string &name)
    {
        const AlgorithmType type = id_to_algorithm_type(name);
        return load_algorithm(type);
    }

    void AlgorithmManager::unload_current_algorithm()
    {
        stop_background_execution();

        std::unique_lock lock(m_algorithm_mutex);

        if (m_current_context.execution)
        {
            LOG_DEBUG(
                "Resetting current algorithm: {}", 
                m_current_context.name
            );

            m_current_context.execution->remove_observer(this);
            m_current_context.execution->reset();
            m_current_context = AlgorithmContext{};

            LOG_DEBUG("Algorithm unloaded");
        }
    }

    void AlgorithmManager::update(double dt)
    {
        std::shared_lock lock(m_algorithm_mutex);
        if (m_current_context.visualizer && m_current_context.is_valid()) 
        {
            m_current_context.visualizer->update(dt);
        }
    }

    void AlgorithmManager::on_step_changed()
    {

        if (!m_current_context.is_valid()) 
            return;

        auto current_step_index = 
            m_current_context.execution->get_current_step_index();

        // Avoiding redundant updates
        if (current_step_index == m_last_step_index)
            return;

        m_last_step_index = current_step_index;

        update_highlight_cache();
    }

    void AlgorithmManager::play()
    {
        if (!m_current_context.execution || 
            m_current_context.execution->is_complete()) 
            return;

        m_is_playing = true;
        m_is_paused = false;

        if (!m_is_executing)
        {
            start_background_execution();
        }
    }

    void AlgorithmManager::pause()
    {
        m_is_playing = false;
        m_is_paused = true;
        LOG_DEBUG("Algorithm playback paused");
    }

    void AlgorithmManager::stop()
    {
        m_is_playing = false;
        m_is_paused = false;

        std::unique_lock lock(m_algorithm_mutex);
        if (m_current_context.execution)
        {
            m_current_context.execution->reset();
        }
    }

    void AlgorithmManager::step_forward() const
    {
        safe_step_forward();
    }

    void AlgorithmManager::step_backward() const
    {
        safe_step_backward();;
    }

    void AlgorithmManager::safe_step_forward() const
    {
        std::unique_lock lock(m_algorithm_mutex);

        if (m_current_context.execution && 
            !m_current_context.execution->is_complete())
        {
            m_current_context.execution->step_forward();

            LOG_DEBUG("Stepped forward to step {}",
                      m_current_context.execution->get_current_step_index());
        }
    }

    void AlgorithmManager::safe_step_backward() const
    {
        std::unique_lock lock(m_algorithm_mutex);

        if (m_current_context.execution && 
            m_current_context.execution->get_current_step_index() > 0)
        {
            m_current_context.execution->step_backward();
            LOG_DEBUG("Stepped backward to step {}",
                       m_current_context.execution->get_current_step_index());
        }
    }

    void AlgorithmManager::start_background_execution()
    {
        LOG_DEBUG("=== START_BACKGROUND_EXECUTION ===");

        if (m_is_executing)
        {
            LOG_DEBUG("Cannot start - conditions not met");
            return;
        }

        m_is_executing = true;
        m_accumulated_time = 0.0;

        // Clean up any previous future
        if (m_executing_future.valid())
        {
            LOG_DEBUG("Cleaning up previous future...");
            auto status = m_executing_future.wait_for(std::chrono::milliseconds(100));
            if (status == std::future_status::timeout) 
            {
                LOG_WARNING("Previous future didn't complete in time");
            }
        }

        try
        {
            LOG_DEBUG("Enqueueing task to thread manager...");
            m_executing_future = m_thread_manager.enqueue_task(
                core::ThreadManager::ThreadType::IO,
                [this]()
            {
                LOG_DEBUG("=== BACKGROUND TASK STARTED EXECUTING ===");
                background_execution_loop();
                LOG_DEBUG("=== BACKGROUND TASK FINISHED ===");
            });
            LOG_DEBUG("Task successfully enqueued to thread manager");
        }
        catch (const std::exception& e)
        {
            m_is_executing = false;
            LOG_ERROR("Failed to enqueue task: {}", e.what());
        }

        LOG_DEBUG("=== START_BACKGROUND_EXECUTION COMPLETED ===");
}

    void AlgorithmManager::stop_background_execution()
    {
        LOG_DEBUG("=== STOP_BACKGROUND_EXECUTION CALLED ===");
        if (!m_is_executing) return;

        m_is_executing = false;
        m_is_playing = false;

        if (m_executing_future.valid())
        {
            LOG_DEBUG("Waiting for future to complete...");
            m_executing_future.wait();
            LOG_DEBUG("Future completed");
        }

        LOG_DEBUG("=== STOP_BACKGROUND_EXECUTION COMPLETED ===");
    }

    void AlgorithmManager::background_execution_loop()
    {
        LOG_DEBUG("Background execution loop STARTED - thread: {}",
                  std::this_thread::get_id());

        auto last_time = std::chrono::high_resolution_clock::now();
        int iteration_count = 0;

        while (m_is_executing && m_thread_manager.should_continue())
        {
            iteration_count++;

            // Debug logging every 100 iterations
            if (iteration_count % 100 == 0) {
                LOG_DEBUG("Background loop iteration: {}, playing: {}, paused: {}",
                         iteration_count, m_is_playing, m_is_paused);
            }

            auto current_time = std::chrono::high_resolution_clock::now();
            double delta_time = std::chrono::duration<double>(current_time - last_time).count();
            last_time = current_time;

            // Only processing if we're actually playing and not paused
            if (m_is_playing && !m_is_paused)
            {
                m_accumulated_time += delta_time * static_cast<double>(m_speed);
                const double step_interval = 0.5;

                // Processing steps based on accumulated time
                while (m_accumulated_time >= step_interval &&
                    m_is_executing &&
                    m_is_playing &&
                    !m_is_paused)
                {
                    bool step_taken = false;

                    ISimpleAlgorithm* algorithm = nullptr;

                    // First safely read pointer
                    {
                        std::shared_lock lock(m_algorithm_mutex);

                        if (m_current_context.is_valid())
                        {
                            algorithm = m_current_context.execution.get();
                        }
                    }

                    if (algorithm && !algorithm->is_complete())
                    {
                        std::unique_lock lock(m_algorithm_mutex);

                        // Ensure algorithm wasn't swapped/unloaded meanwhile
                        if (m_current_context.execution.get() == algorithm)
                        {
                            algorithm->step_forward();
                            m_accumulated_time -= step_interval;
                            step_taken = true;

                            if (algorithm->get_current_step_index() % 10 == 0)
                            {
                                LOG_DEBUG(
                                    "Algorithm progress: {}/{}",
                                    algorithm->get_current_step_index(),
                                    algorithm->get_step_count()
                                );
                            }
                        }
                    }
                    else
                    {
                        m_is_playing = false;
                        LOG_DEBUG("Algorithm completed, stopping playback");
                        break;
                    }

                    if (!step_taken)
                    {
                        break;
                    }
                }
            }
            else
            {
                // Not playing or paused - reseting accumulated time back to prevent burst when resuming
                m_accumulated_time = 0.0;
            }

            // Use a proper sleep to prevent CPU spinning
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60Hz
        }

        m_is_executing = false;
        LOG_DEBUG("Background execution loop ENDED after {} iterations", iteration_count);
    }

    void AlgorithmManager::set_speed(const float speed)
    {
        m_speed = std::max(0.1f, std::min(speed, 5.0f));
    }

    void AlgorithmManager::set_data(const std::vector<int>& data)
    {
        stop_background_execution();

        std::unique_lock lock(m_algorithm_mutex);

        if (m_current_context.execution && !data.empty())
        {
            m_current_context.execution->initialize(data);
        }
    }

    std::vector<int> AlgorithmManager::generate_random_data(
        size_t size, 
        int max_value)
    {
        std::vector<int> data;
        data.reserve(size);
        
        // Using proper random number generation
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, max_value);
        
        for (size_t i = 0; i < size; ++i)
        {
            data.push_back(dis(gen));
        }
        
        return data;
    }

    void AlgorithmManager::generate_and_set_random_data(
        size_t size, 
        int max_value)
    {
        set_data(generate_random_data(size, max_value));
    }

    void AlgorithmManager::update_highlight_cache()
    {
        std::unique_lock unique_lock(m_cached_mutex);

        m_cached_code_highlights.clear();
        m_cached_pseudocode_display = {};

        if (!m_current_context.is_valid())
            return;

        auto* json_algorithm = dynamic_cast<JsonAlgorithmBase*>(
            m_current_context.execution.get()
        );

        if (!json_algorithm)
            return;

        auto current_step = m_current_context.execution->get_current_step();

        std::string operation_id;

        switch (current_step.metadata.operation_type)
        {
            case AlgorithmStepOperation::INIT:          
                operation_id = "init"; break;
            // Bubble Sort Operations
            case AlgorithmStepOperation::LOOP_OUTER:    
                operation_id = "outer_loop"; break;
            case AlgorithmStepOperation::LOOP_INNER:    
                operation_id = "inner_loop"; break;
            case AlgorithmStepOperation::PASS_COMPLETE: 
                operation_id = "pass_complete"; break;
            // Quick Sort Operations
            case AlgorithmStepOperation::PARTITION_START:       
                operation_id = "partition_start"; break;
            case AlgorithmStepOperation::PIVOT_SELECTED:          
                operation_id = "pivot_selected"; break;
            case AlgorithmStepOperation::PARTITION_SCAN:          
                operation_id = "partition_scan"; break;
            case AlgorithmStepOperation::PARTITION_SWAP:          
                operation_id = "partition_swap"; break;
            case AlgorithmStepOperation::PARTITION_COMPLETE:          
                operation_id = "partition_complete"; break;
            case AlgorithmStepOperation::RECURSIVE_CALL:          
                operation_id = "recursive_call"; break;
            case AlgorithmStepOperation::RECURSIVE_CALL_RIGHT:          
                operation_id = "recursive_call_right"; break;
            // General Operations
            case AlgorithmStepOperation::COMPARE:       
                operation_id = "compare"; break;
            case AlgorithmStepOperation::SWAP:          
                operation_id = "swap"; break;
            case AlgorithmStepOperation::COMPLETED:      
                operation_id = "completed"; break;
            
            default:                                    
                operation_id = "unknown"; break;
        }

        m_cached_code_highlights =
            json_algorithm->generate_highlights(operation_id, current_step);

        const auto& description = m_current_context.metadata->get_description();

        if (description.has_pseudocode())
        {
            m_cached_pseudocode_display.lines = description.pseudocode;
        }
        else 
        {
            m_cached_pseudocode_display.lines = {"Pseudocode not available for this algorithm."};
            return;
        }

        for (const auto& highlight : m_cached_code_highlights)
        {
            if (!highlight.is_active)
                continue;

            m_cached_pseudocode_display.highlighted_lines.push_back(
                highlight.line_number
            );

            if (!highlight.index_variables.empty())
                m_cached_pseudocode_display.line_index_values[highlight.line_number] =
                    highlight.index_variables;

            if (!highlight.variable_values.empty())
                m_cached_pseudocode_display.line_variable_values[highlight.line_number] =
                    highlight.variable_values;
        }
    }

    const std::vector<CodeHighlight>& 
    AlgorithmManager::get_current_code_highlights() const
    {
        std::shared_lock lock(m_algorithm_mutex);
        return m_cached_code_highlights;
    }

    const PseudocodeDisplay&
    AlgorithmManager::get_current_pseudocode_with_highlights() const
    {
        std::shared_lock lock(m_algorithm_mutex);
        return m_cached_pseudocode_display;
    }

    AlgorithmType AlgorithmManager::get_current_algorithm_type() const noexcept
    {
        std::shared_lock lock(m_algorithm_mutex);

        if (!m_current_context.is_valid())
        {
            return AlgorithmType::UNKNOWN;
        }

        return m_current_context.metadata->get_type();
    }
    

    IAlgorithmVisualizer* AlgorithmManager::get_current_visualizer() const
    {
        std::shared_lock lock(m_algorithm_mutex);
        return m_current_context.visualizer.get();
    }

    ISimpleAlgorithm* AlgorithmManager::get_current_algorithm() const
    {
        std::shared_lock lock(m_algorithm_mutex);
        return m_current_context.execution.get();
    }

    const IAlgorithmMetadata *AlgorithmManager::get_current_metadata() const
    {
        return m_current_context.metadata;
    }

    const std::string& AlgorithmManager::get_current_algorithm_name() const
    {
        std::shared_lock lock(m_algorithm_mutex);
        return m_current_context.metadata->get_display_name();
    }

    bool AlgorithmManager::is_playing() const
    {
        return m_is_playing && !m_is_paused;
    }

    bool AlgorithmManager::is_paused() const
    {
        return m_is_paused;
    }

    bool AlgorithmManager::is_executing() const
    {
        return m_is_executing;
    }

    float AlgorithmManager::get_speed() const
    {
        return m_speed.load();
    }


} // namespace c2l::algorithms
