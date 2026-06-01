//
// Created by Akhmad on 5/30/26.
//

#ifndef CODE2LOGIC_SHARED_GRAPH_STATE_HPP
#define CODE2LOGIC_SHARED_GRAPH_STATE_HPP

#include "algorithms/visualizers/graph_based_visualizer.hpp"
#include "algorithms/visualizers/graph_data.hpp"

#include <functional>
#include <random>

#include "graph_layout_engine.hpp"

namespace c2l::algorithms
{
    /**
     * @brief Shared graph state for both left and right algorithms in comparison mode
     */
    struct SharedGraphState
    {
        GraphData graph;
        GraphLayoutEngine::LayoutType current_layout{GraphLayoutEngine::LayoutType::HIERARCHICAL};

        // Callback when graph changes
        std::function<void()> on_graph_changed;

        SharedGraphState()
        {
            create_default_graph();
        }

        void create_default_graph()
        {
            graph.clear();
            for (int i = 0; i < 6; ++i)
                graph.add_node("Node " + std::to_string(i), (i + 1) * 10);
            graph.add_edge(0, 1);
            graph.add_edge(0, 2);
            graph.add_edge(0, 3);
            graph.add_edge(1, 4);
            graph.add_edge(2, 4);
            graph.add_edge(3, 5);
            graph.add_edge(4, 5);
            graph.start_node = 0;
            graph.target_node = 5;

            // Compute initial layout
            compute_layout(ImVec2(800, 600));
        }

        void compute_layout(
            const ImVec2& bounds,
            GraphLayoutEngine::LayoutType type = GraphLayoutEngine::LayoutType::HIERARCHICAL)
        {
            current_layout = type;
            GraphLayoutEngine engine;
            engine.compute_layout(graph, current_layout, bounds);
        }

        void add_node(const std::string& label = "", int value = 0)
        {
            graph.add_node(label, value);
        }

        bool add_edge(size_t from, size_t to, int weight = 1)
        {
            return graph.add_edge(from, to, weight);
        }

        void remove_node(size_t id)
        {
            graph.remove_node(id);
        }

        void set_start_node(size_t start)
        {
            graph.start_node = start;
        }

        void set_target_node(size_t target)
        {
            graph.target_node = target;
        }

        void set_directed(bool directed)
        {
            graph.is_directed = directed;
        }

        void set_weighted(bool weighted)
        {
            graph.is_weighted = weighted;
        }

        // Preset graphs
        void create_path_graph(int node_count = 5)
        {
            graph.clear();
            for (int i = 0; i < node_count; ++i)
                graph.add_node("N" + std::to_string(i), i);
            for (int i = 0; i < node_count - 1; ++i)
                graph.add_edge(i, i + 1);
            graph.start_node = 0;
            graph.target_node = node_count - 1;

            // Compute layout for path graph
            compute_layout(ImVec2(800, 600), GraphLayoutEngine::LayoutType::HIERARCHICAL);
        }

        void create_cycle_graph(int node_count = 6)
        {
            graph.clear();
            for (int i = 0; i < node_count; ++i)
                graph.add_node("N" + std::to_string(i), i);
            for (int i = 0; i < node_count; ++i)
                graph.add_edge(i, (i + 1) % node_count);
            graph.start_node = 0;
            graph.target_node = node_count / 2;

            // Circular layout is best for cycles
            compute_layout(ImVec2(800, 600), GraphLayoutEngine::LayoutType::CIRCULAR);
        }

        void create_complete_graph(int node_count = 5)
        {
            graph.clear();
            for (int i = 0; i < node_count; ++i)
                graph.add_node("N" + std::to_string(i), i);
            for (int i = 0; i < node_count; ++i)
                for (int j = i + 1; j < node_count; ++j)
                    graph.add_edge(i, j);
            graph.start_node = 0;
            graph.target_node = node_count - 1;

            // Circular layout works well for complete graphs
            compute_layout(ImVec2(800, 600), GraphLayoutEngine::LayoutType::CIRCULAR);
        }

        void create_star_graph(int node_count = 6)
        {
            graph.clear();
            for (int i = 0; i < node_count; ++i)
                graph.add_node("N" + std::to_string(i), i);
            for (int i = 1; i < node_count; ++i)
                graph.add_edge(0, i);
            graph.start_node = 0;
            graph.target_node = node_count - 1;

            // Radial tree works well for stars
            compute_layout(ImVec2(800, 600), GraphLayoutEngine::LayoutType::RADIAL_TREE);
        }

        void create_binary_tree(int node_count = 15)
        {
            graph.clear();
            for (int i = 0; i < node_count; ++i)
                graph.add_node("N" + std::to_string(i), i);

            for (size_t i = 0; i < static_cast<size_t>(node_count); ++i)
            {
                size_t left_child = 2 * i + 1;
                size_t right_child = 2 * i + 2;

                if (left_child < static_cast<size_t>(node_count))
                    graph.add_edge(i, left_child);
                if (right_child < static_cast<size_t>(node_count))
                    graph.add_edge(i, right_child);
            }
            graph.start_node = 0;
            graph.target_node = node_count - 1;

            // Hierarchical layout for trees
            compute_layout(ImVec2(800, 600), GraphLayoutEngine::LayoutType::HIERARCHICAL);
        }

        void clear()
        {
            graph.clear();
            graph.start_node = 0;
            graph.target_node.reset();
        }

        std::vector<int> get_node_values() const
        {
            return graph.get_node_values();
        }

        std::vector<std::vector<size_t>> get_adjacency_matrix() const
        {
            return graph.get_adjacency_matrix();
        }
    };
}

#endif // CODE2LOGIC_SHARED_GRAPH_STATE_HPP