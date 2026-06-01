#ifndef CODE2LOGIC_GRAPH_LAYOUT_ENGINE_HPP
#define CODE2LOGIC_GRAPH_LAYOUT_ENGINE_HPP

#include "graph_data.hpp"
#include <imgui.h>
#include <random>
#include <functional>
#include <queue>
#include <cmath>

namespace c2l::algorithms
{
    class GraphLayoutEngine
    {
    public:
        enum class LayoutType
        {
            FORCE_DIRECTED, CIRCULAR, HIERARCHICAL, GRID, RADIAL_TREE,
            CONCENTRIC, SPECTRAL, SPIRAL, BIPARTITE, VERTICAL_TREE,
            HORIZONTAL_TREE, RANDOM
        };

        struct Params
        {
            float spring_constant{0.05f};
            float repulsion_constant{100.0f};
            float gravity{0.05f};
            float damping{0.95f};
            float temperature{1.0f};
            Params() {}

        };

        void compute_layout(
            GraphData& graph,
            LayoutType type,
            const ImVec2& bounds,
            const Params& params = {}
        );
        void update_force_directed(
            GraphData& graph,
            float delta_time,
            const Params& params
        );

    private:
        std::mt19937 m_rng{std::random_device{}()};
        void compute_circular(GraphData& graph, const ImVec2& bounds);
        void compute_grid(GraphData& graph, const ImVec2& bounds);
        void compute_hierarchical(GraphData& graph, const ImVec2& bounds);
        void compute_radial_tree(GraphData& graph, const ImVec2& bounds);
        void compute_concentric(GraphData& graph, const ImVec2& bounds);
        void compute_spectral(GraphData& graph, const ImVec2& bounds);
        void compute_spiral(GraphData& graph, const ImVec2& bounds);
        void compute_bipartite(GraphData& graph, const ImVec2& bounds);
        void compute_vertical_tree(GraphData& graph, const ImVec2& bounds);
        void compute_horizontal_tree(GraphData& graph, const ImVec2& bounds);
        void compute_random(GraphData& graph, const ImVec2& bounds);
    };
}

#endif