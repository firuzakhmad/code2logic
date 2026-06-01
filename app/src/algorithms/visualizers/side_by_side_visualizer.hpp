#ifndef CODE2LOGIC_SIDE_BY_SIDE_VISUALIZER_HPP
#define CODE2LOGIC_SIDE_BY_SIDE_VISUALIZER_HPP

#include "algorithms/visualizers/i_algorithm_visualizer.hpp"
#include "algorithms/visualizers/array_based_visualizer.hpp"
#include "algorithms/managers/parallel_comparison_manager.hpp"
#include "algorithms/managers/parallel_algorithm.hpp"

#include "imgui.h"
#include "shared_graph_state.hpp"
#include "shared_grid_state.hpp"

namespace c2l::algorithms
{
    /**
     * @brief Visualizes two algorithms side by side for comparison
     */
    class SideBySideVisualizer
    {
    public:
        SideBySideVisualizer() = default;
        ~SideBySideVisualizer() = default;

        void initialize(
            const ParallelAlgorithm& left,
            const ParallelAlgorithm& right,
            SharedGridState* shared_grid_state = nullptr);

        void render();

        void render_metrics_panel(
            ISimpleAlgorithm *algorithm,
            const IAlgorithmMetadata *metadata);

        void render_splitter(float height);

        void update(double dt);

        void sync_shared_grid_to_visualizer(IAlgorithmVisualizer& visualizer);
        void sync_shared_graph_to_visualizer(IAlgorithmVisualizer& visualizer);

        // Configuration
        void set_split_position(float position) { m_split_position = position; }
        void set_show_metrics(bool show) { m_show_metrics = show; }
        void set_show_labels(bool show) { m_show_labels = show; }


        void render_algorithm_side(
            const char *side_name,
            const ParallelAlgorithm& algo,
            const ImVec2 &size);

        const ParallelAlgorithm* m_left{nullptr};
        const ParallelAlgorithm* m_right{nullptr};

        SharedGridState* m_shared_grid_state{nullptr};
        SharedGraphState* m_shared_graph_state{nullptr};

        bool m_show_grid_lines{true};
        bool m_show_weights{true};
        bool m_show_coordinates{false};

        bool m_show_edge_weights{true};
        bool m_show_node_labels{true};
        bool m_show_node_ids{true};
        bool m_show_distances{true};

        float m_split_position                      {0.5f};
        bool m_show_metrics                         {true};
        bool m_show_labels                          {true};

        // Animation
        double m_animation_time                     {0.0};
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_SIDE_BY_SIDE_VISUALIZER_HPP