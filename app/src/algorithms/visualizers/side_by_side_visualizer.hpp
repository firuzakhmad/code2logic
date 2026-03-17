#ifndef CODE2LOGIC_SIDE_BY_SIDE_VISUALIZER_HPP
#define CODE2LOGIC_SIDE_BY_SIDE_VISUALIZER_HPP

#include "algorithms/visualizers/i_algorithm_visualizer.hpp"
#include "algorithms/visualizers/array_based_visualizer.hpp"

#include "imgui.h"

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
            ISimpleAlgorithm *left_algo,
            const IAlgorithmMetadata *left_meta,
            ISimpleAlgorithm *right_algo,
            const IAlgorithmMetadata *right_meta);

        void render();
        void update(double dt);

        // Configuration
        void set_split_position(float position) { m_split_position = position; }
        void set_show_metrics(bool show) { m_show_metrics = show; }
        void set_show_labels(bool show) { m_show_labels = show; }


        void render_algorithm_side(
            const char *side_name,
            ISimpleAlgorithm *algorithm,
            const IAlgorithmMetadata *metadata,
            const ImVec2 &size,
            bool is_left);

        void render_comparison_metrics();

        ISimpleAlgorithm* m_left_algorithm{nullptr};
        const IAlgorithmMetadata *m_left_metadata{nullptr};
        ISimpleAlgorithm* m_right_algorithm{nullptr};
        const IAlgorithmMetadata *m_right_metadata{nullptr};

        ArrayBasedVisualizer m_left_visualizer;
        ArrayBasedVisualizer m_right_visualizer;

        float m_split_position{0.5f};
        bool m_show_metrics{true};
        bool m_show_labels{true};

        // Animation
        double m_animation_time{0.0};
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_SIDE_BY_SIDE_VISUALIZER_HPP