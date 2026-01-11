//
// Created by Akhmad on 11/8/25.
//

#ifndef CODE2LOGIC_ALGORITHM_MANAGER_HPP
#define CODE2LOGIC_ALGORITHM_MANAGER_HPP

#include "algorithms/i_simple_algorithm.hpp"
#include "core/utils/thread_manager/thread_manager.hpp"
#include "algorithms/algorithm_variable.hpp"
#include "algorithms/algorithm_types.hpp"
#include "algorithms/algorithm_observer.hpp"
#include "algorithms/code_highlight.hpp"
#include "algorithms/i_algorithm_visualizer.hpp"

#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include <string>


namespace c2l::algorithms
{
    class AlgorithmManager final : public AlgorithmObserver
    {
    public:
        explicit AlgorithmManager(core::ThreadManager& thread_manager);
        ~AlgorithmManager() override;

        // Algorithm registration
        void register_algorithm(
            AlgorithmType type,
            std::unique_ptr<ISimpleAlgorithm> algorithm);
        void register_algorithm(
            const std::string& name,
            std::unique_ptr<ISimpleAlgorithm> algorithm);
        void unregister_algorithm(AlgorithmType type);
        void unregister_algorithm(const std::string& name);

        // Algorithm control
        bool load_algorithm(AlgorithmType type);
        bool load_algorithm(const std::string& name);
        void unload_current_algorithm();

        void update(double dt);
        void render();
        // Playback control (thread-safe)
        void play();
        void pause();
        void stop();
        void step_forward() const;
        void step_backward() const;

        void on_step_changed() override;

        // Background execution control
        void start_background_execution();
        void stop_background_execution();

        // Setters
        void set_speed(float speed);
        void set_data(const std::vector<int>& data);

        std::vector<int> generate_random_data();
        void generate_and_set_random_data();


        // Getters (thread-safe)
        [[nodiscard]] const std::vector<AlgorithmType>& get_available_algorithm_types() const;
        [[nodiscard]] const AlgorithmType& get_current_algorithm_type() const;
        [[nodiscard]] const std::string& get_current_algorithm_name() const;
        [[nodiscard]] const std::vector<std::string>& get_algorithm_names() const;
        [[nodiscard]] const std::vector<std::string>& get_available_algorithms() const;
        [[nodiscard]] const std::vector<CodeHighlight>& get_code_highlights() const;
        [[nodiscard]] const std::unordered_map<std::string, AlgorithmType>& get_name_to_type_map() const;
        [[nodiscard]] const std::vector<CodeHighlight>& get_current_code_highlights() const;

        [[nodiscard]] std::vector<AlgorithmType> get_algorithms_by_category(AlgorithmCategory category) const;
        [[nodiscard]] PseudocodeDisplay get_current_pseudocode_with_highlights() const;

        [[nodiscard]] ISimpleAlgorithm* get_current_algorithm() const;
        [[nodiscard]] IAlgorithmVisualizer* get_current_visualizer() const;
        // Code visualization

        [[nodiscard]] bool is_playing() const;
        [[nodiscard]] bool is_paused() const;
        [[nodiscard]] bool is_executing() const;
        [[nodiscard]] float get_speed() const;

    private:
        template<typename T>
        std::optional<T> extract_variable(const AlgorithmStep& step,
                                      const std::string& key) const
        {
            return step.metadata.get<T>(key);
        }

        std::optional<AlgorithmVariable> extract_variable_object(
            const AlgorithmStep& step,
            const std::string& key) const;

        std::unordered_map<std::string, std::string> extract_all_variables(
            const AlgorithmStep& step) const;

        void initialize_algorithms();
        void background_execution_loop();
        void safe_step_forward() const;
        void safe_step_backward() const;
        std::unique_ptr<IAlgorithmVisualizer> create_visualizer(AlgorithmType type);

        void setup_highlight_strategies();

        // Code generation methods
        void generate_bubble_sort_code_highlights();
        void generate_quick_sort_code_highlights();
        void generate_binary_search_code_highlights();
        void generate_linear_search_code_highlights();
        void generate_bfs_code_highlights();
        void generate_dfs_code_highlights();

        PseudocodeDisplay generate_bubble_sort_pseudocode_display() const;
        PseudocodeDisplay generate_quick_sort_pseudocode_display() const;
        PseudocodeDisplay generate_binary_search_pseudocode_display() const;
        PseudocodeDisplay generate_linear_search_pseudocode_display() const;


        core::ThreadManager& m_thread_manager;

        std::unordered_map<AlgorithmType, std::unique_ptr<ISimpleAlgorithm>> m_algorithms;
        std::unordered_map<std::string, AlgorithmType> m_name_to_type_map;
        std::vector<AlgorithmType> m_algorithm_types;
        std::vector<std::string> m_algorithm_names;

        ISimpleAlgorithm* m_current_algorithm   {nullptr};

        AlgorithmType m_current_algorithm_type  {AlgorithmType::BUBBLE_SORT};
        std::string m_current_algorithm_name;

        // Visualization system
        std::unordered_map<AlgorithmType, std::unique_ptr<IAlgorithmVisualizer>> m_visualizers;
        IAlgorithmVisualizer* m_current_visualizer {nullptr};

        std::atomic<bool> m_is_playing          {false};
        std::atomic<bool> m_is_paused           {false};
        std::atomic<bool> m_is_executing        {false};
        std::atomic<float> m_speed              {1.0f};

        // Background execution
        std::future<void> m_executing_future;
        std::atomic<double> m_accumulated_time  {0.0};

        // Thread synchronization
        mutable std::shared_mutex m_algorithm_mutex;

        // Visualization state
        std::unordered_map<AlgorithmType, std::function<void()>> m_highlight_strategies;
        std::vector<CodeHighlight> m_current_highlights;
        size_t m_last_highlighted_step          {std::numeric_limits<size_t>::max()};
        std::string m_current_pseudocode;
    };
}

#endif //CODE2LOGIC_ALGORITHM_MANAGER_HPP