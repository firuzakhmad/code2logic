//
// Created by Akhmad on 11/8/25.
//

#include "algorithm_manager.hpp"
#include "algorithms/bubble_sort.hpp"
#include "algorithms/quick_sort.hpp"
#include "algorithms/array_based_visualizer.hpp"
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
        core::JsonConfigManager& json_config_manager)
        : m_thread_manager{thread_manager}
        , m_json_config_manager{json_config_manager}
    {
        initialize_algorithms();
        LOG_INFO(
            "AlgorithmManager initialized with {} algorithms", 
            m_algorithms.size()
        );
    }

    AlgorithmManager::~AlgorithmManager()
    {
        LOG_DEBUG("AlgorithmManager shutting down...");
        stop_background_execution();
        unload_current_algorithm();
        LOG_DEBUG("AlgorithmManager destroyed");
    }


    void AlgorithmManager::initialize_algorithms()
    {
        // Registering JSON-driven algorithms
        register_algorithm(
            AlgorithmType::BUBBLE_SORT, 
            std::make_unique<BubbleSort>(m_json_config_manager)
        );

        register_algorithm(
            AlgorithmType::QUICK_SORT, 
            std::make_unique<QuickSort>(m_json_config_manager)
        );

        LOG_INFO(
            "Registered {} algorithms from JSON configuration", 
            m_algorithms.size()
        );
    }

    std::unique_ptr<IAlgorithmVisualizer> AlgorithmManager::create_visualizer(
        AlgorithmType type,
        const VisualizationConfig&)
    {
        auto category = algorithm_category(type);

        switch (category) {
            case AlgorithmCategory::SORTING:
            case AlgorithmCategory::SEARCHING:
                return std::make_unique<ArrayBasedVisualizer>();
                
            case AlgorithmCategory::GRAPH:
                // return std::make_unique<GraphBasedVisualizer>(config);
                
            case AlgorithmCategory::TREE:
                // return std::make_unique<TreeBasedVisualizer>(config);
                
            default:
                return std::make_unique<ArrayBasedVisualizer>();
        }
    }

    void AlgorithmManager::register_algorithm(
        AlgorithmType type, 
        std::unique_ptr<ISimpleAlgorithm> algorithm,
        const VisualizationConfig& visualization_config)
    {
        std::unique_lock lock(m_algorithm_mutex);

        if (m_algorithms.find(type) != m_algorithms.end())
        {
            LOG_WARNING(
                "Algorithm type '{}' already registered, overwriting",
                algorithm_display_name(type)
            );
        }

        m_algorithms[type] = std::move(algorithm);

        // Updating available types list
        if (std::find(m_algorithm_types.begin(), 
                      m_algorithm_types.end(), 
                      type) == m_algorithm_types.end())
        {
            m_algorithm_types.push_back(type);
        }

        // Updating name mapping for backward compatibility
        std::string name = std::string(algorithm_id(type));
        m_name_to_type_map[name] = type;

        // Updating legacy names list
        if (std::find(m_algorithm_names.begin(), 
                      m_algorithm_names.end(), 
                      name) == m_algorithm_names.end())
        {
            m_algorithm_names.push_back(name);
        }

        // Creating visualizer for the registered algorithm
        m_visualizers[type] = create_visualizer(type, visualization_config);

        LOG_DEBUG(
            "Registered algorithm: {} (Type: {})", 
            name, 
            static_cast<int>(type)
        );
    }

    void AlgorithmManager::register_algorithm(
        const std::string &id,
        std::unique_ptr<ISimpleAlgorithm> algorithm,
        const VisualizationConfig& visualization_config)
    {
        AlgorithmType type = id_to_algorithm_type(id);
        register_algorithm(
            type, 
            std::move(algorithm), 
            visualization_config
        );
    }

    void AlgorithmManager::unregister_algorithm(AlgorithmType type)
    {
        std::unique_lock lock(m_algorithm_mutex);

        if (m_current_context.metadata &&
            m_current_context.metadata->get_type() == type)
        {
            unload_current_algorithm();
        }

        if (m_algorithms.find(type) != m_algorithms.end())
        {
            m_algorithms.erase(type);

            // Removing from types list
            auto type_it = std::find(m_algorithm_types.begin(), m_algorithm_types.end(), type);
            if (type_it != m_algorithm_types.end())
            {
                m_algorithm_types.erase(type_it);
            }

            // Removing from legacy names
            std::string name = std::string(algorithm_id(type));
            auto name_it = std::find(m_algorithm_names.begin(), m_algorithm_names.end(), name);
            if (name_it != m_algorithm_names.end())
            {
                m_algorithm_names.erase(name_it);
            }

            // Remove from name mapping
            m_name_to_type_map.erase(name);

            LOG_DEBUG("Unregistered algorithm: {} (Type: {})", name, static_cast<int>(type));
        }
    }

    void AlgorithmManager::unregister_algorithm(const std::string &name)
    {
        AlgorithmType type = id_to_algorithm_type(name);
        unregister_algorithm(type);
    }

    bool AlgorithmManager::load_algorithm(AlgorithmType type)
    {
        stop_background_execution();

        {
            std::shared_lock lock(m_algorithm_mutex);

            if (m_algorithms.find(type) == m_algorithms.end())
            {
                LOG_ERROR(
                    "Algorithm type '{}' not registered", 
                    algorithm_display_name(type)
                );
                return false;
            }
        }

        unload_current_algorithm();

        {
            std::unique_lock lock(m_algorithm_mutex);

            auto alg_it = m_algorithms.find(type);
            if (alg_it == m_algorithms.end()) 
            {
                LOG_ERROR(
                    "Algorithm type '{}' disappeared during load", 
                    algorithm_display_name(type)
                );
                return false;
            }


            m_current_context.execution = alg_it->second.get();
            m_current_context.metadata = dynamic_cast<IAlgorithmMetadata*>(alg_it->second.get());

            auto viz_it = m_visualizers.find(type);
            if (viz_it != m_visualizers.end())
            {
                m_current_context.visualizer = viz_it->second.get();
                m_current_context.visualizer->initialize(
                    m_current_context.execution,
                    m_current_context.metadata
                );
            }

            m_current_context.type = type;
            m_current_context.name = algorithm_display_name(type);

            if (!m_current_context.metadata)
            {
                LOG_ERROR("Algorithm does not implement IAlgorithmMetadata");
                return false;
            }

            if (!m_current_context.metadata->is_valid())
            {
                LOG_ERROR("Algorithm metadata is invalid");
                return false;
            }

            // Attaching observer
            if (m_current_context.execution)
            {
                m_current_context.execution->add_observer(this);
            }

            // Initializing random data
            auto default_data = generate_random_data();
            m_current_context.execution->initialize(default_data);

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
                            algorithm = m_current_context.execution;
                        }
                    }

                    if (algorithm && !algorithm->is_complete())
                    {
                        std::unique_lock lock(m_algorithm_mutex);

                        // Ensure algorithm wasn't swapped/unloaded meanwhile
                        if (m_current_context.execution == algorithm)
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
            m_current_context.execution
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


    const std::vector<AlgorithmType>& AlgorithmManager::get_available_algorithm_types() const
    {
        std::shared_lock lock(m_algorithm_mutex);
        return m_algorithm_types;
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
        return m_current_context.visualizer;
    }


    std::vector<AlgorithmType> AlgorithmManager::get_algorithm_types_by_category(
        const AlgorithmCategory category) const
    {
        std::shared_lock lock(m_algorithm_mutex);
        std::vector<AlgorithmType> result;

        for (auto type : m_algorithm_types)
        {
            if (algorithm_category(type) == category)
            {
                result.push_back(type);
            }
        }

        return result;
    }

    AlgorithmManager::CategorizedAlgorithms
    AlgorithmManager::get_available_categorized_algorithms() const
    {
        CategorizedAlgorithms result;

        for (AlgorithmType type : m_algorithm_types)
        {
            if (const auto* info = algorithms::get_algorithm_info(type))
                result[info->display_category].push_back(info);
        }

        return result;
    }

    ISimpleAlgorithm* AlgorithmManager::get_current_algorithm() const
    {
        std::shared_lock lock(m_algorithm_mutex);
        return m_current_context.execution;
    }


    IAlgorithmMetadata* AlgorithmManager::get_current_metadata() const 
    {
        return m_current_context.metadata;
    }

    const std::string& AlgorithmManager::get_current_algorithm_name() const
    {
        std::shared_lock lock(m_algorithm_mutex);
        return m_current_context.metadata->get_display_name();
    }

    const std::vector<std::string>& AlgorithmManager::get_available_algorithm_names() const
    {
        std::shared_lock lock(m_algorithm_mutex);
        return m_algorithm_names;
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
