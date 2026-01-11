//
// Created by Akhmad on 11/8/25.
//

#include "algorithm_manager.hpp"
#include "algorithms/bubble_sort_algorithm.hpp"
#include "algorithms/quick_sort_algorithm.hpp"
#include "algorithms/array_based_visualizer.hpp"
#include "core/utils/logger/logger.hpp"

#include <utility>

#include <glm/glm.hpp>
#include "imgui.h"

namespace c2l::algorithms
{
    AlgorithmManager::AlgorithmManager(core::ThreadManager& thread_manager)
        : m_thread_manager{thread_manager}
    {
        initialize_algorithms();
        setup_highlight_strategies();
    }

    AlgorithmManager::~AlgorithmManager()
    {
        stop_background_execution();
        unload_current_algorithm();
    }


    void AlgorithmManager::initialize_algorithms()
    {
        register_algorithm(AlgorithmType::BUBBLE_SORT, std::make_unique<BubbleSortAlgorithm>());
        register_algorithm(AlgorithmType::QUICK_SORT, std::make_unique<QuickSortAlgorithm>());
        LOG_INFO("Registered {} algorithms", m_algorithms.size());
    }

    void AlgorithmManager::register_algorithm(AlgorithmType type, std::unique_ptr<ISimpleAlgorithm> algorithm)
    {
        std::unique_lock lock(m_algorithm_mutex);

        if (m_algorithms.find(type) != m_algorithms.end())
        {
            LOG_WARNING("Algorithm type '{}' already registered, overwriting",
                                   algorithm_type_to_string(type));
        }

        m_algorithms[type] = std::move(algorithm);

        // Updating available types list
        if (std::find(m_algorithm_types.begin(), m_algorithm_types.end(), type) == m_algorithm_types.end())
        {
            m_algorithm_types.push_back(type);
        }

        // Updating name mapping for backward compatibility
        std::string name = algorithm_type_to_string(type);
        m_name_to_type_map[name] = type;

        // Updating legacy names list
        if (std::find(m_algorithm_names.begin(), m_algorithm_names.end(), name) == m_algorithm_names.end())
        {
            m_algorithm_names.push_back(name);
        }

        m_visualizers[type] = create_visualizer(type);

        LOG_DEBUG("Registered algorithm: {} (Type: {})", name, static_cast<int>(type));
    }

    void AlgorithmManager::register_algorithm(const std::string &name,
                                              std::unique_ptr<ISimpleAlgorithm> algorithm)
    {
        AlgorithmType type = string_to_algorithm_type(name);
        register_algorithm(type, std::move(algorithm));
    }

    void AlgorithmManager::unregister_algorithm(AlgorithmType type)
    {
        std::unique_lock lock(m_algorithm_mutex);

        if (m_current_algorithm_type == type)
        {
            unload_current_algorithm();
        }

        if (m_algorithms.find(type) != m_algorithms.end())
        {
            m_algorithms.erase(type);

            // Remove from types list
            auto type_it = std::find(m_algorithm_types.begin(), m_algorithm_types.end(), type);
            if (type_it != m_algorithm_types.end())
            {
                m_algorithm_types.erase(type_it);
            }

            // Remove from legacy names
            std::string name = algorithm_type_to_string(type);
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
        AlgorithmType type = string_to_algorithm_type(name);
        unregister_algorithm(type);
    }

    bool AlgorithmManager::load_algorithm(AlgorithmType type)
    {
        stop_background_execution();

        {
            std::shared_lock lock(m_algorithm_mutex);
            if (m_algorithms.find(type) == m_algorithms.end())
            {
                LOG_ERROR("Algorithm type '{}' not registered", algorithm_type_to_string(type));
                return false;
            }
        }

        unload_current_algorithm();

        {
            std::unique_lock lock(m_algorithm_mutex);
            auto it = m_algorithms.find(type);
            if (it == m_algorithms.end()) {
                LOG_ERROR("Algorithm type '{}' disappeared during load", algorithm_type_to_string(type));
                return false;
            }

            m_current_algorithm = it->second.get();
            m_current_algorithm_type = type;
            m_current_algorithm_name = algorithm_type_to_string(type);

            if (m_current_algorithm)
            {
                m_current_algorithm->add_observer(this);
            }

            // Set up visualizer
            auto viz_it = m_visualizers.find(type);
            if (viz_it != m_visualizers.end())
            {
                m_current_visualizer = viz_it->second.get();
                m_current_visualizer->initialize(m_current_algorithm);
            }

            auto default_data = generate_random_data();
            m_current_algorithm->initialize(default_data);
        }

        return true;
    }

    bool AlgorithmManager::load_algorithm(const std::string &name)
    {
        const AlgorithmType type = string_to_algorithm_type(name);
        return load_algorithm(type);
    }

    void AlgorithmManager::unload_current_algorithm()
    {
        stop_background_execution();

        std::unique_lock lock(m_algorithm_mutex);

        if (m_current_algorithm)
        {
            LOG_DEBUG("Resetting current algorithm: {}", m_current_algorithm_name);

            m_current_algorithm->remove_observer(this);

            m_current_algorithm->reset();
            m_current_algorithm = nullptr;
            m_current_algorithm_type = AlgorithmType::BUBBLE_SORT;
            m_current_algorithm_name.clear();
            LOG_DEBUG("Algorithm unloaded");
        }
    }

    void AlgorithmManager::update(double dt)
    {
        if (m_current_visualizer) {
            m_current_visualizer->update(dt);
        }
    }

    void AlgorithmManager::render()
    {
    }

    std::unique_ptr<IAlgorithmVisualizer> AlgorithmManager::create_visualizer(AlgorithmType type)
    {
        auto viz_type = get_visualization_type(type);

        switch (viz_type) {
            case VisualizationType::ARRAY_BASED:
                return std::make_unique<ArrayBasedVisualizer>();
            case VisualizationType::GRAPH_BASED:
                // return std::make_unique<GraphBasedVisualizer>(); // Implement later
            case VisualizationType::TREE_BASED:
                // return std::make_unique<TreeBasedVisualizer>(); // Implement later
            default:
                return std::make_unique<ArrayBasedVisualizer>();
        }
    }

    void AlgorithmManager::on_step_changed()
    {
        // Just call the right strategy
        if (const auto it = m_highlight_strategies.find(m_current_algorithm_type);
            it != m_highlight_strategies.end())
        {
            it->second();
        }
    }

    void AlgorithmManager::setup_highlight_strategies()
    {
        // Map algorithm types to highlight functions
        m_highlight_strategies[AlgorithmType::BUBBLE_SORT] = [this]()
        {
            generate_bubble_sort_code_highlights();
        };
        m_highlight_strategies[AlgorithmType::QUICK_SORT] = [this]()
        {
            generate_quick_sort_code_highlights();
        };
        // ... add others
    }



    void AlgorithmManager::play()
    {
        if (!m_current_algorithm || m_current_algorithm->is_complete()) return;

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
        if (m_current_algorithm)
        {
            m_current_algorithm->reset();
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

        if (m_current_algorithm && !m_current_algorithm->is_complete())
        {
            m_current_algorithm->step_forward();

            LOG_DEBUG("Stepped forward to step {}",
                      m_current_algorithm->get_current_step_index());
        }
    }

    void AlgorithmManager::safe_step_backward() const
    {
        std::unique_lock lock(m_algorithm_mutex);

        if (m_current_algorithm && m_current_algorithm->get_current_step_index() > 0)
        {
            m_current_algorithm->step_backward();
            LOG_DEBUG("Stepped backward to step {}",
                       m_current_algorithm->get_current_step_index());
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
            if (status == std::future_status::timeout) {
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

            // Only process if we're actually playing and not paused
            if (m_is_playing && !m_is_paused)
            {
                m_accumulated_time += delta_time * static_cast<double>(m_speed);
                const double step_interval = 0.5;

                // Process steps based on accumulated time
                while (m_accumulated_time >= step_interval &&
                       m_is_executing &&
                       m_is_playing &&
                       !m_is_paused)
                {
                    bool step_taken = false;

                    {
                        std::unique_lock lock(m_algorithm_mutex);

                        if (m_current_algorithm && !m_current_algorithm->is_complete())
                        {
                            m_current_algorithm->step_forward();
                            m_accumulated_time -= step_interval;
                            step_taken = true;

                            // Log progress occasionally
                            if (m_current_algorithm->get_current_step_index() % 10 == 0)
                            {
                                LOG_DEBUG("Algorithm progress: {}/{}",
                                         m_current_algorithm->get_current_step_index(),
                                         m_current_algorithm->get_step_count());
                            }
                        }
                        else
                        {
                            // Algorithm completed
                            m_is_playing = false;
                            LOG_DEBUG("Algorithm completed, stopping playback");
                            break;
                        }
                    }

                    if (!step_taken) {
                        break; // No algorithm loaded or other issue
                    }
                }
            }
            else
            {
                // Not playing or paused - reset accumulated time to prevent burst when resuming
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

        if (m_current_algorithm && !data.empty())
        {
            m_current_algorithm->initialize(data);
        }
    }

    std::vector<int> AlgorithmManager::generate_random_data()
    {
        std::vector<int> new_data;
        new_data.reserve(15);
        for (int i = 0; i < 15; ++i)
        {
            new_data.emplace_back(rand() % 200 + 1);
        }

        return new_data;
    }

    void AlgorithmManager::generate_and_set_random_data()
    {
        std::vector<int> new_data;
        new_data.reserve(15);
        for (int i = 0; i < 15; ++i)
        {
            new_data.emplace_back(rand() % 200 + 1);
        }

        set_data(new_data);
    }

    // Code highlighting implementations
    void AlgorithmManager::generate_bubble_sort_code_highlights()
    {
        if (!m_current_algorithm) return;
        m_current_highlights.clear();

        const AlgorithmStep step = m_current_algorithm->get_current_step();

        auto i  = extract_variable<size_t>(step, "i");
        auto j  = extract_variable<size_t>(step, "j");
        auto aj = extract_variable<int>(step, "arr[j]");
        auto aj1= extract_variable<int>(step, "arr[j+1]");

        std::unordered_map<std::string, std::string> indices;
        std::unordered_map<std::string, std::string> values;

        if (i)  indices["i"] = std::to_string(*i);
        if (j)  indices["j"] = std::to_string(*j);
        if (aj) values["arr[j]"] = std::to_string(*aj);
        if (aj1)values["arr[j+1]"] = std::to_string(*aj1);

        switch (step.metadata.operation_type)
        {
            case AlgorithmStepOperation::INIT:
                m_current_highlights.emplace_back(
                    1,
                    "procedure bubble_sort(arr):",
                    "Starting bubble sort algorithm",
                    indices,
                    values, true);
                break;

            case AlgorithmStepOperation::LOOP_OUTER:
                m_current_highlights.emplace_back(
                    7,
                    "for (size_t i = 0; i < n - 1; ++i)",
                    "Outer loop iteration",
                    indices, values, true);
                break;

            case AlgorithmStepOperation::LOOP_INNER:
                m_current_highlights.emplace_back(
                    11,
                    "for (size_t j = 0; j < n - i - 1; ++j)",
                    "Inner loop iteration",
                    indices, values, true);
                break;

            case AlgorithmStepOperation::COMPARE:
                m_current_highlights.emplace_back(
                    13,
                    "if (arr[j] > arr[j + 1])",
                    "Comparing adjacent elements",
                    indices, values, true);
                break;

            case AlgorithmStepOperation::SWAP:
                m_current_highlights.emplace_back(
                    15,
                    "if (arr[j] > arr[j + 1])",
                    "Comparison condition met",
                    indices, values, true);

                m_current_highlights.emplace_back(
                    16,
                    "std::swap(arr[j], arr[j + 1])",
                    "Swapping elements",
                    indices, values, true);
                break;

            case AlgorithmStepOperation::PASS_COMPLETE:
                m_current_highlights.emplace_back(
                    7,
                    "for (size_t i = 0; i < n - 1; ++i)",
                    "Pass completed",
                    indices, values, true);
                break;

            case AlgorithmStepOperation::FINISHED:
                m_current_highlights.emplace_back(
                    21,
                    "procedure bubble_sort(arr):",
                    "Algorithm completed",
                    indices, values, true);
                break;

            default:
                break;
        }
    }

    void AlgorithmManager::generate_quick_sort_code_highlights()
    {
        if (!m_current_algorithm) return;
        m_current_highlights.clear();

        auto current_step = m_current_algorithm->get_current_step();
        std::string description = current_step.description;

        std::unordered_map<std::string, std::string> vars;

        // Extract values from description
        if (description.find("low =") != std::string::npos || description.find("high =") != std::string::npos) {
            // Parse for low and high values
            size_t low_pos = description.find("low =");
            size_t high_pos = description.find("high =");
            if (low_pos != std::string::npos) {
                std::string low_str = description.substr(low_pos + 5);
                size_t comma_pos = low_str.find(',');
                if (comma_pos != std::string::npos) {
                    vars["low"] = low_str.substr(0, comma_pos);
                }
            }
            if (high_pos != std::string::npos) {
                std::string high_str = description.substr(high_pos + 6);
                size_t end_pos = high_str.find(')');
                if (end_pos != std::string::npos) {
                    vars["high"] = high_str.substr(0, end_pos);
                }
            }
        }

        // Determine which lines to highlight
        if (description.find("Initial array") != std::string::npos) {
            m_current_highlights.emplace_back(
                1,
                "void quickSort(vector<int>& arr, int low, int high)",
                "Starting QuickSort algorithm",
                vars,
                std::unordered_map<std::string, std::string>(),
                true);
        }
        else if (description.find("Recursive call") != std::string::npos) {
            m_current_highlights.emplace_back(
                3,
                "    if (low < high)",
                "Entering recursive call",
                vars,
                std::unordered_map<std::string, std::string>(),
                true);
            m_current_highlights.emplace_back(
                5,
                "        int pi = partition(arr, low, high)",
                "Calling partition function",
                vars,
                std::unordered_map<std::string, std::string>(),
                true);
        }
        else if (description.find("Starting partition") != std::string::npos)
        {
            m_current_highlights.emplace_back(
                10,
                "int partition(vector<int>& arr, int low, int high)",
                "Starting partition process",
                vars,
                std::unordered_map<std::string, std::string>(),
                true);
            m_current_highlights.emplace_back(
                12,
                "    int pivot = arr[high]",
                "Selecting pivot element",
                vars,
                std::unordered_map<std::string, std::string>(),
                true);
        }
        else if (description.find("Comparing") != std::string::npos)
        {
            m_current_highlights.emplace_back(
                15,
                "    for (int j = low; j <= high - 1; j++)",
                "Iterating through partition",
                vars,
                std::unordered_map<std::string, std::string>(),
                true);
            m_current_highlights.emplace_back(
                17,
                "        if (arr[j] <= pivot)",
                "Comparing element with pivot",
                vars,
                std::unordered_map<std::string, std::string>(),
                true);
        }
        else if (description.find("Swapped") != std::string::npos)
        {
            m_current_highlights.emplace_back(
                19,
                "            swap(arr[i], arr[j])",
                "Swapping elements",
                vars,
                std::unordered_map<std::string, std::string>(),
                true);
        }
        else if (description.find("Placed pivot") != std::string::npos)
        {
            m_current_highlights.emplace_back(
                23,
                "    swap(arr[i + 1], arr[high])",
                "Placing pivot in final position",
                vars,
                std::unordered_map<std::string, std::string>(),
                true);
        }
        else if (description.find("completely sorted") != std::string::npos)
        {
            m_current_highlights.emplace_back(
                1,
                "void quickSort(vector<int>& arr, int low, int high)",
                "Algorithm completed - array is sorted",
                vars,
                std::unordered_map<std::string, std::string>(),
                true);
        }
    }

    void AlgorithmManager::generate_binary_search_code_highlights()
    {
        m_current_highlights.clear();
        if (!m_current_algorithm) return;

        auto current_step = m_current_algorithm->get_current_step();

        if (current_step.description.find("Comparing") != std::string::npos) {
            m_current_highlights.emplace_back(4, "    mid = low + (high - low) / 2", "Calculating midpoint", true);
            m_current_highlights.emplace_back(5, "    if arr[mid] == target:", "Checking if midpoint is target", true);
        } else if (current_step.description.find("Found") != std::string::npos) {
            m_current_highlights.emplace_back(6, "        return mid", "Target found at midpoint", true);
        }
    }

    void AlgorithmManager::generate_linear_search_code_highlights()
    {
        m_current_highlights.clear();
        if (!m_current_algorithm) return;

        auto current_step = m_current_algorithm->get_current_step();

        if (current_step.description.find("Checking") != std::string::npos) {
            m_current_highlights.emplace_back(2, "    for i = 0 to n-1:", "Iterating through array", true);
            m_current_highlights.emplace_back(3, "        if arr[i] == target:", "Checking current element", true);
        }
    }

    void AlgorithmManager::generate_bfs_code_highlights(){}
    void AlgorithmManager::generate_dfs_code_highlights(){}


    PseudocodeDisplay AlgorithmManager::generate_bubble_sort_pseudocode_display() const
    {
        PseudocodeDisplay display;
        display.lines = {
            "template<typename T>",
            "void bubble_sort(std::vector<T>& arr)",
            "{",
            "       size_t n = arr.size();",
            "       bool swapped;",
            "",
            "       for (size_t i = 0; i < n - 1; ++i)",
            "       {",
            "               swapped = false;",
            "",
            "               for (size_t j = 0; j < n - i - 1; ++j)",
            "               {",
            "                       if (arr[j] > arr[j + 1])",
            "                       {",
            "                               std::swap(arr[j], arr[j + 1]);",
            "                               swapped = true;",
            "                       }",
            "               }",
            "",
            "               // If no swapping occurred, array is sorted",
            "               if (!swapped) break;",
            "       }",
            "}",
            ""
        };
        return display;
    }

    PseudocodeDisplay AlgorithmManager::generate_quick_sort_pseudocode_display() const
    {
        PseudocodeDisplay display;
        display.lines = {
            "template<typename T>",
            "void quick_sort(std::vector<T>& arr, int low, int high)",
            "{",
            "       if (low < high)",
            "       {",
            "           pi = partition(arr, low, high);",
            "",
            "           quick_sort(arr, low, pi - 1);",
            "           quick_sort(arr, pi + 1, high);",
            "       }",
            "}",
            "",
            "",
            "template<typename T>",
            "int partition(std::vector<T>& arr, int low, int high)",
            "{",
            "       pivot = arr[high];",
            "",
            "       int i = low - 1;",
            "",
            "       for (j = low; j <= high - 1; j++)",
            "       {",
            "               if (arr[j] <= pivot)",
            "               {",
            "                       if (i != j)",
            "                       {",
            "                               i++;",
            "                               swap(arr[i], arr[j]);",
            "                       }",
            "               }",
            "       }",
            "",
            "       swap(arr[i + 1], arr[high]);",
            "",
            "       return (i + 1)",
            "}"
        };
        return display;
    }

    PseudocodeDisplay AlgorithmManager::generate_binary_search_pseudocode_display() const
    {
        PseudocodeDisplay display;
        display.lines = {
            "function binarySearch(arr, target):",
            "    low = 0",
            "    high = length(arr) - 1",
            "",
            "    while low <= high:",
            "        mid = low + (high - low) // 2",
            "",
            "        if arr[mid] == target:",
            "            return mid          # Target found",
            "        else if arr[mid] < target:",
            "            low = mid + 1       # Search right half",
            "        else:",
            "            high = mid - 1      # Search left half",
            "",
            "    return -1                   # Target not found"
        };
        return display;
    }

    PseudocodeDisplay AlgorithmManager::generate_linear_search_pseudocode_display() const
    {
        PseudocodeDisplay display;
        display.lines = {
            "function linearSearch(arr, target):",
            "    n = length(arr)",
            "",
            "    for i = 0 to n-1:",
            "        if arr[i] == target:",
            "            return i            # Target found at index i",
            "",
            "    return -1                   # Target not found"
        };
        return display;
    }

    PseudocodeDisplay AlgorithmManager::get_current_pseudocode_with_highlights() const
    {
        PseudocodeDisplay display;

        switch (m_current_algorithm_type)
        {
            case AlgorithmType::BUBBLE_SORT:
                display = generate_bubble_sort_pseudocode_display();
                break;
            case AlgorithmType::QUICK_SORT:
                display = generate_quick_sort_pseudocode_display();
                break;
            case AlgorithmType::BINARY_SEARCH:
                display = generate_binary_search_pseudocode_display();
                break;
            case AlgorithmType::LINEAR_SEARCH:
                display = generate_linear_search_pseudocode_display();
                break;
            default:
                display.lines = {"Pseudocode not available for this algorithm."};
                break;
        }

        // Add current highlights to the display
        for (const auto& highlight : m_current_highlights)
        {
            if (highlight.is_active)
            {
                display.highlighted_lines.push_back(highlight.line_number);
                if (!highlight.variable_values.empty())
                {
                    display.line_index_values[highlight.line_number] = highlight.index_variables;
                    display.line_variable_values[highlight.line_number] = highlight.variable_values;

                }
            }
        }

        return display;
    }

    const std::unordered_map<std::string, AlgorithmType>& AlgorithmManager::get_name_to_type_map() const
    {
        std::shared_lock lock(m_algorithm_mutex);
        return m_name_to_type_map;
    }


    const std::vector<CodeHighlight>& AlgorithmManager::get_current_code_highlights() const
    {
        std::shared_lock lock(m_algorithm_mutex);
        return m_current_highlights;
    }

    const std::vector<AlgorithmType>& AlgorithmManager::get_available_algorithm_types() const
    {
        std::shared_lock lock(m_algorithm_mutex);
        return m_algorithm_types;
    }

    const std::vector<std::string>& AlgorithmManager::get_available_algorithms() const
    {
        std::shared_lock lock(m_algorithm_mutex);
        return m_algorithm_names;
    }

    const AlgorithmType& AlgorithmManager::get_current_algorithm_type() const
    {
        std::shared_lock lock(m_algorithm_mutex);
        return m_current_algorithm_type;
    }

    IAlgorithmVisualizer *AlgorithmManager::get_current_visualizer() const
    {
        std::shared_lock lock(m_algorithm_mutex);
        return m_current_visualizer;
    }


    std::vector<AlgorithmType> AlgorithmManager::get_algorithms_by_category(const AlgorithmCategory category) const
    {
        std::shared_lock lock(m_algorithm_mutex);
        std::vector<AlgorithmType> result;

        for (auto type : m_algorithm_types)
        {
            if (get_algorithm_category(type) == category)
            {
                result.push_back(type);
            }
        }

        return result;
    }

    ISimpleAlgorithm* AlgorithmManager::get_current_algorithm() const
    {
        std::shared_lock lock(m_algorithm_mutex);
        return m_current_algorithm;
    }

    const std::string& AlgorithmManager::get_current_algorithm_name() const
    {
        std::shared_lock lock(m_algorithm_mutex);
        return m_current_algorithm_name;
    }

    const std::vector<CodeHighlight>& AlgorithmManager::get_code_highlights() const
    {
        std::shared_lock lock(m_algorithm_mutex);
        return m_current_highlights;
    }

    const std::vector<std::string>& AlgorithmManager::get_algorithm_names() const
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

    std::optional<AlgorithmVariable> AlgorithmManager::extract_variable_object(
            const AlgorithmStep& step, const std::string& key) const
    {
        return step.metadata.get_variable(key);
    }

    std::unordered_map<std::string, std::string> AlgorithmManager::extract_all_variables(
        const AlgorithmStep& step) const
    {
        std::unordered_map<std::string, std::string> result;
        for (const auto& [key, var] : step.metadata.variables) {
            result[key] = var.to_string();
        }
        return result;
    }


} // namespace c2l::algorithms
