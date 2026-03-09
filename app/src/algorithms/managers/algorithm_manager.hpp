//
// Created by Akhmad on 11/8/25.
//

#ifndef CODE2LOGIC_ALGORITHM_MANAGER_HPP
#define CODE2LOGIC_ALGORITHM_MANAGER_HPP

#include "algorithms/json_algorithm_base.hpp"
#include "core/utils/thread_manager/thread_manager.hpp"
#include "algorithms/algorithm_variable.hpp"
#include "algorithms/algorithm_types.hpp"
#include "algorithms/algorithm_observer.hpp"
#include "algorithms/code_highlight.hpp"
#include "algorithms/i_algorithm_visualizer.hpp"
#include "core/json_config_manager/json_config_manager.hpp"

#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include <string>
#include <optional>



namespace c2l::algorithms
{
    /**
     * @brief Enhanced Algorithm Manager with full JSON metadata support
     * 
     * This manager handles algorithm lifecycle, step execution, visualization,
     * and provides comprehensive access to JSON-driven metadata.
     */
    class AlgorithmManager final : public AlgorithmObserver
    {
    public:
        using AlgorithmPtr = std::unique_ptr<ISimpleAlgorithm>;
        using MetadataPtr = IAlgorithmMetadata*;
        using VisualizaterPtr = std::unique_ptr<IAlgorithmVisualizer>;

        using CategorizedAlgorithms = std::unordered_map<
            std::string_view, 
            std::vector<const AlgorithmInfo*>
        >;

        struct AlgorithmContext 
        {
            ISimpleAlgorithm* execution         {nullptr};
            IAlgorithmMetadata* metadata        {nullptr};
            IAlgorithmVisualizer* visualizer    {nullptr};
            std::string name;
            AlgorithmType type                  {AlgorithmType::UNKNOWN};

            [[nodiscard]] bool is_valid() const noexcept
            {
                return execution != nullptr && metadata != nullptr;
            }

            void reset()
            {
                execution = nullptr;
                metadata = nullptr;
                visualizer = nullptr;
                name.clear();
                type = AlgorithmType::UNKNOWN;
            }

        };


        explicit AlgorithmManager(
            core::ThreadManager& thread_manager,
            core::JsonConfigManager& json_config_manager);

        ~AlgorithmManager() override;

        AlgorithmManager(const AlgorithmManager&) = delete;
        AlgorithmManager& operator=(const AlgorithmManager&) = delete;
        AlgorithmManager(AlgorithmManager&&) noexcept = delete;
        AlgorithmManager& operator=(AlgorithmManager&&) noexcept = delete;


        // Algorithm registration
        void register_algorithm(
            AlgorithmType type,
            std::unique_ptr<ISimpleAlgorithm> algorithm,
            const VisualizationConfig& visualization_config = {}
        );

        void register_algorithm(
            const std::string& name,
            std::unique_ptr<ISimpleAlgorithm> algorithm,
            const VisualizationConfig& visualization_config = {}
        );

        void unregister_algorithm(AlgorithmType type);
        void unregister_algorithm(const std::string& name);

        // Algorithm control
        bool load_algorithm(AlgorithmType type);
        bool load_algorithm(const std::string& name);
        void unload_current_algorithm();

        void update(double dt);
        
        // Playback control (thread-safe)
        void play();
        void pause();
        void stop();
        void step_forward() const;
        void step_backward() const;

        // Background execution control
        void start_background_execution();
        void stop_background_execution();

        // Setters
        void set_speed(float speed);
        void set_data(const std::vector<int>& data);

        std::vector<int> generate_random_data(size_t size = 15, int max_value = 200);
        void generate_and_set_random_data(size_t size = 15, int max_value = 200);

        // Step management
        void on_step_changed() override;


        // Getters (thread-safe)
        /**
         * @brief Get code highlights for current step using JSON-driven highlighting
         */
        [[nodiscard]] const std::vector<CodeHighlight>& get_current_code_highlights() const;

        /**
         * @brief Get pseudocode display with current step highlights
         */
        [[nodiscard]] const PseudocodeDisplay& get_current_pseudocode_with_highlights() const;

        // Algorithm lists
        [[nodiscard]] const std::vector<AlgorithmType>& get_available_algorithm_types() const;
        [[nodiscard]] const std::vector<std::string>& get_available_algorithm_names() const;
        [[nodiscard]] CategorizedAlgorithms get_available_categorized_algorithms() const;
        
        // Current algorithm state
        // [[nodiscard]] const AlgorithmContext& get_current_context() const noexcept;
        [[nodiscard]] ISimpleAlgorithm* get_current_algorithm() const;
        [[nodiscard]] IAlgorithmMetadata* get_current_metadata() const;
        [[nodiscard]] IAlgorithmVisualizer* get_current_visualizer() const;
        [[nodiscard]] const std::string& get_current_algorithm_name() const;
        [[nodiscard]] AlgorithmType get_current_algorithm_type() const noexcept;

        // Metadata queries
        [[nodiscard]] std::vector<AlgorithmType> get_algorithm_types_by_category(AlgorithmCategory category) const;

        // Playback state
        [[nodiscard]] bool is_playing() const;
        [[nodiscard]] bool is_paused() const;
        [[nodiscard]] bool is_executing() const;
        [[nodiscard]] float get_speed() const;

    private:
        void initialize_algorithms();

        /**
         * @brief Create appropriate visualizer for algorithm type
         */
        std::unique_ptr<IAlgorithmVisualizer> create_visualizer(
            AlgorithmType type,
            const VisualizationConfig& config);

        // Background execution
        void background_execution_loop();
        void safe_step_forward() const;
        void safe_step_backward() const;

        void update_highlight_cache();


        core::ThreadManager& m_thread_manager;
        core::JsonConfigManager& m_json_config_manager;

        std::unordered_map<AlgorithmType, std::unique_ptr<ISimpleAlgorithm>> m_algorithms;
        std::unordered_map<std::string, AlgorithmType> m_name_to_type_map;
        std::vector<AlgorithmType> m_algorithm_types;
        std::vector<std::string> m_algorithm_names;

        AlgorithmContext m_current_context;

        // Visualization system
        std::unordered_map<AlgorithmType, std::unique_ptr<IAlgorithmVisualizer>> m_visualizers;

        std::atomic<bool> m_is_playing          {false};
        std::atomic<bool> m_is_paused           {false};
        std::atomic<bool> m_is_executing        {false};
        std::atomic<float> m_speed              {1.0f};

        // Background execution
        std::future<void> m_executing_future;
        std::atomic<double> m_accumulated_time  {0.0};

        // Thread synchronization
        mutable std::shared_mutex m_algorithm_mutex;
        mutable std::shared_mutex m_cached_mutex;

        // // Visualization state
        std::vector<CodeHighlight> m_cached_code_highlights;
        PseudocodeDisplay m_cached_pseudocode_display;

        size_t m_last_step_index                {std::numeric_limits<size_t>::max()};
    };
}

#endif //CODE2LOGIC_ALGORITHM_MANAGER_HPP