#ifndef CODE2LOGIC_GRAPH_BASED_VISUALIZER_HPP
#define CODE2LOGIC_GRAPH_BASED_VISUALIZER_HPP

#include "algorithms/visualizers/i_algorithm_visualizer.hpp"
#include "algorithms/core/algorithm_metadata_types.hpp"
#include "algorithms/visualizers/visualization_style.hpp"
#include "ui/managers/icon_manager.hpp"
#include "algorithms/visualizers/shared_graph_state.hpp"
#include "algorithms/visualizers/graph_data.hpp"
#include "algorithms/visualizers/graph_layout_engine.hpp"
#include "algorithms/visualizers/shared_graph_state.hpp"

#include <imgui.h>
#include <functional>
#include <random>

#include "algorithms/bfs.hpp"

namespace c2l::algorithms
{
    struct SharedGraphState;


    class GraphBasedVisualizer final : public IAlgorithmVisualizer
    {
    public:
        GraphBasedVisualizer(
            ui::managers::IconManager& icon_manager,
            const VisualizationConfig& config = {});
        ~GraphBasedVisualizer() override = default;

        void initialize(
            ISimpleAlgorithm* execution,
            const IAlgorithmMetadata* metadata,
            const bool show_sidebar_controller
        ) override;
        void update(double delta_time) override;
        void render() override;
        VisualizationType get_visualization_type() const override;
        bool supports_algorithm(const AlgorithmType& type) const override;

        void set_visualization_style(VisualizationStyle style) override;
        void set_graph_layout(const std::string& layout);
        void set_show_edge_weights(bool show);
        void set_show_node_labels(bool show);
        void set_show_node_ids(bool show);
        void set_show_distances(bool show);

        void sync_from_shared_state(const SharedGraphState& state);

    private:
        void render_graph_editor_panel();
        void render_node_edit_panel();
        void render_edge_edit_panel();
        void render_properties_panel();
        void render_algorithm_config_panel();
        void render_controls_toolbar();
        void draw_graph();
        void handle_mouse_interaction();
        void handle_keyboard_shortcuts();
        void sync_algorithm_with_graph();

        ui::managers::IconManager& m_icon_manager;
        const VisualizationConfig& m_config;

        ISimpleAlgorithm *m_execution{nullptr};
        const IAlgorithmMetadata* m_metadata{nullptr};

        GraphData m_graph;
        GraphLayoutEngine m_layout_engine;
        GraphLayoutEngine::LayoutType m_current_layout{
            GraphLayoutEngine::LayoutType::HIERARCHICAL
        };

        // Callback
        std::function<void()> m_on_graph_changed;

        // UI State
        bool m_show_graph_editor{true};
        bool m_show_properties{true};
        bool m_show_algorithm_config{true};
        bool m_show_edge_weights{true};
        bool m_show_node_labels{true};
        bool m_show_node_ids{true};
        bool m_show_distances{true};

        // Interaction
        ImVec2 m_view_offset{0, 0};
        float m_zoom{1.0f};
        bool m_is_panning{false};
        ImVec2 m_pan_start{0, 0};
        size_t m_selected_node{static_cast<size_t>(-1)};
        size_t m_hovered_node{static_cast<size_t>(-1)};
        bool m_is_dragging_node{false};

        // Temp input for node/edge creation
        char m_new_node_label[64] = "";
        int m_new_node_value = 0;
        int m_new_edge_from = 0;
        int m_new_edge_to = 0;
        int m_new_edge_weight = 1;
        size_t m_node_to_connect{static_cast<size_t>(-1)};

        // Animation
        float m_animation_time{0.0f};
        float m_layout_update_timer{0.0f};

        static constexpr const char* LAYOUT_NAMES[] = {
            "Force Directed", "Circular", "Hierarchical", "Grid",
            "Radial Tree", "Concentric", "Spectral", "Spiral",
            "Bipartite", "Vertical Tree", "Horizontal Tree", "Random"
        };

        static constexpr float MIN_ZOOM = 0.2f;
        static constexpr float MAX_ZOOM = 5.0f;
    };
}

#endif