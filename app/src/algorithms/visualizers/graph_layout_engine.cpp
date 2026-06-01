//
// Created by Akhmad on 5/30/26.
//

#include "algorithms/visualizers/graph_layout_engine.hpp"

namespace c2l::algorithms
{
    // GraphLayoutEngine Implementation
    void GraphLayoutEngine::compute_layout(
        GraphData& graph,
        LayoutType type,
        const ImVec2& bounds,
        const Params& params)
    {
        if (graph.node_count() < 2) return;

        switch (type)
        {
            case LayoutType::FORCE_DIRECTED:
                compute_random(graph, bounds);
                for (int i = 0; i < 100; ++i)
                    update_force_directed(graph, 0.033f, params);
                break;
            case LayoutType::CIRCULAR:
                compute_circular(graph, bounds); break;
            case LayoutType::GRID:
                compute_grid(graph, bounds); break;
            case LayoutType::HIERARCHICAL:
                compute_hierarchical(graph, bounds); break;
            case LayoutType::RADIAL_TREE:
                compute_radial_tree(graph, bounds); break;
            case LayoutType::CONCENTRIC:
                compute_concentric(graph, bounds); break;
            case LayoutType::SPECTRAL:
                compute_spectral(graph, bounds); break;
            case LayoutType::SPIRAL:
                compute_spiral(graph, bounds); break;
            case LayoutType::BIPARTITE:
                compute_bipartite(graph, bounds); break;
            case LayoutType::VERTICAL_TREE:
                compute_vertical_tree(graph, bounds); break;
            case LayoutType::HORIZONTAL_TREE:
                compute_horizontal_tree(graph, bounds); break;
            case LayoutType::RANDOM:
                compute_random(graph, bounds); break;
        }
    }

    void GraphLayoutEngine::update_force_directed(
        GraphData& graph,
        float delta_time,
        const Params& params)
    {
        const float dt = std::min(delta_time, 0.033f);

        // Reseting forces
        std::unordered_map<size_t, ImVec2> forces;
        for (auto& [id, node] : graph.nodes)
            forces[id] = ImVec2(0, 0);

        // Repulsion
        std::vector<size_t> node_ids;
        for (const auto& [id, _] : graph.nodes) node_ids.push_back(id);

        for (size_t i = 0; i < node_ids.size(); ++i)
        {
            for (size_t j = i + 1; j < node_ids.size(); ++j)
            {
                size_t id1 = node_ids[i];
                size_t id2 = node_ids[j];
                ImVec2 diff(graph.nodes[id1].position.x - graph.nodes[id2].position.x,
                           graph.nodes[id1].position.y - graph.nodes[id2].position.y);
                float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y);
                if (distance < 0.01f)
                    distance = 0.01f;

                float force_mag = params.repulsion_constant / (distance * distance);
                ImVec2 force(diff.x / distance * force_mag, diff.y / distance * force_mag);
                forces[id1].x += force.x; forces[id1].y += force.y;
                forces[id2].x -= force.x; forces[id2].y -= force.y;
            }
        }

        // Attraction for edges
        for (const auto& edge : graph.edges)
        {
            auto& node1 = graph.nodes[edge.from];
            auto& node2 = graph.nodes[edge.to];
            ImVec2 diff(
                node1.position.x - node2.position.x,
                node1.position.y - node2.position.y
            );
            float distance = std::sqrt(
                diff.x * diff.x + diff.y * diff.y
            );
            float spring_force = params.spring_constant * (distance - 0.1f);
            ImVec2 force(
                diff.x / distance * spring_force,
                diff.y / distance * spring_force
            );
            forces[edge.from].x -= force.x;
            forces[edge.from].y -= force.y;
            forces[edge.to].x += force.x;
            forces[edge.to].y += force.y;
        }

        // Applying forces
        for (auto& [id, node] : graph.nodes)
        {
            if (node.is_fixed)
                continue;
            forces[id].x -= (node.position.x - 0.5f) * params.gravity;
            forces[id].y -= (node.position.y - 0.5f) * params.gravity;

            node.velocity.x = (node.velocity.x + forces[id].x * dt) * params.damping;
            node.velocity.y = (node.velocity.y + forces[id].y * dt) * params.damping;
            node.position.x += node.velocity.x * dt * params.temperature;
            node.position.y += node.velocity.y * dt * params.temperature;
            node.position.x = std::clamp(node.position.x, 0.02f, 0.98f);
            node.position.y = std::clamp(node.position.y, 0.02f, 0.98f);
        }
    }

    void GraphLayoutEngine::compute_circular(
        GraphData& graph,
        const ImVec2& bounds)
    {
        float angle_step = 2.0f * 3.14159f / graph.node_count();
        size_t idx = 0;
        for (auto& [id, node] : graph.nodes)
        {
            float angle = idx++ * angle_step;
            node.position = ImVec2(
                0.5f + std::cos(angle) * 0.4f,
                0.5f + std::sin(angle) * 0.4f
            );
            node.velocity = ImVec2(0, 0);
        }
    }

    void GraphLayoutEngine::compute_grid(
        GraphData& graph,
        const ImVec2& bounds)
    {
        int cols = static_cast<int>(std::ceil(std::sqrt(graph.node_count())));
        int rows = static_cast<int>(std::ceil(static_cast<float>(graph.node_count()) / cols));
        float cell_w = 1.0f / cols, cell_h = 1.0f / rows;
        size_t idx = 0;
        for (auto& [id, node] : graph.nodes)
        {
            int row = idx / cols, col = idx % cols;
            node.position = ImVec2(
                (col + 0.5f) * cell_w,
                (row + 0.5f) * cell_h
            );
            idx++;
        }
    }

    void GraphLayoutEngine::compute_hierarchical(
        GraphData& graph,
        const ImVec2& bounds)
    {
        // BFS layering from start node
        std::unordered_map<size_t, int> level;
        std::queue<size_t> q;
        size_t start = graph.start_node;
        if (!graph.has_node(start) &&
            !graph.nodes.empty()) start = graph.nodes.begin()->first;

        level[start] = 0;
        q.push(start);

        while (!q.empty())
        {
            size_t current = q.front(); q.pop();
            for (size_t neighbor : graph.adjacency_list[current])
                if (level.find(neighbor) == level.end())
                {
                    level[neighbor] = level[current] + 1; q.push(neighbor);
                }
        }

        std::unordered_map<int, std::vector<size_t>> level_nodes;
        int max_level = 0;
        for (const auto& [id, lvl] : level)
        {
            level_nodes[lvl].push_back(id);
            max_level = std::max(max_level, lvl);
        }

        float level_h = 1.0f / (max_level + 2);
        for (const auto& [lvl, nodes] : level_nodes)
        {
            float y = 0.1f + lvl * level_h;
            float spacing = 1.0f / (nodes.size() + 1);
            for (size_t j = 0; j < nodes.size(); ++j)
                graph.nodes[nodes[j]].position = ImVec2(spacing * (j + 1), y);
        }
    }

    void GraphLayoutEngine::compute_radial_tree(
        GraphData& graph,
        const ImVec2& bounds)
    {
        // Same as hierarchical but radial
        compute_hierarchical(graph, bounds);
        // Convert to radial
        for (auto& [id, node] : graph.nodes)
        {
            float angle = node.position.x * 2.0f * 3.14159f;
            float radius = 0.2f + node.position.y * 0.6f;
            node.position = ImVec2(
                0.5f + std::cos(angle) * radius,
                0.5f + std::sin(angle) * radius
            );
        }
    }

    void GraphLayoutEngine::compute_concentric(
        GraphData& graph,
        const ImVec2& bounds)
    {
        // Sort by degree
        std::vector<std::pair<size_t, int>> degrees;
        for (const auto& [id, node] : graph.nodes)
            degrees.emplace_back(
                id,
                graph.adjacency_list[id].size()
            );
        std::sort(
            degrees.begin(),
            degrees.end(),
            [](auto& a, auto& b)
            {
                return a.second > b.second;
            }
        );

        int rings = std::min(5, (int)graph.node_count());
        int per_ring = graph.node_count() / rings;
        size_t idx = 0;

        for (int r = 0; r < rings && idx < graph.node_count(); ++r)
        {
            float radius = 0.15f + r * 0.17f;
            int ring_nodes = (r == rings - 1) ? graph.node_count() - idx : per_ring;
            float step = 2.0f * 3.14159f / ring_nodes;
            for (int i = 0; i < ring_nodes && idx < graph.node_count(); ++i)
            {
                size_t id = degrees[idx].first;
                float angle = i * step;
                graph.nodes[id].position = ImVec2(
                    0.5f + std::cos(angle) * radius,
                    0.5f + std::sin(angle) * radius
                );
                idx++;
            }
        }
    }

    void GraphLayoutEngine::compute_spectral(
        GraphData& graph,
        const ImVec2& bounds)
    {
        compute_circular(graph, bounds);
        // Sort by degree and redistribute
        std::vector<std::pair<size_t, int>> degrees;
        for (const auto& [id, node] : graph.nodes)
            degrees.emplace_back(id, graph.adjacency_list[id].size());
        std::sort(
            degrees.begin(),
            degrees.end(),
            [](auto& a, auto& b)
            {
                return a.second > b.second;
            }
        );

        float step = 2.0f * 3.14159f / graph.node_count();
        for (size_t i = 0; i < degrees.size(); ++i)
        {
            float angle = i * step;
            float radius = 0.2f + (degrees[i].second / 10.0f) * 0.3f;
            graph.nodes[degrees[i].first].position = ImVec2(
                0.5f + std::cos(angle) * radius,
                0.5f + std::sin(angle) * radius
            );
        }
    }

    void GraphLayoutEngine::compute_spiral(
        GraphData& graph,
        const ImVec2& bounds)
    {
        float golden = (1.0f + std::sqrt(5.0f)) / 2.0f;
        size_t idx = 0;
        for (auto& [id, node] : graph.nodes)
        {
            float theta = 2.0f * 3.14159f * idx / golden;
            float radius = std::sqrt((float)(idx + 1) / graph.node_count()) * 0.45f;
            node.position = ImVec2(
                0.5f + std::cos(theta) * radius,
                0.5f + std::sin(theta) * radius
            );
            idx++;
        }
    }

    void GraphLayoutEngine::compute_bipartite(
        GraphData& graph,
        const ImVec2& bounds)
    {
        // 2-coloring
        std::unordered_map<size_t, int> color;
        std::queue<size_t> q;
        if (!graph.nodes.empty())
        {
            size_t start = graph.nodes.begin()->first;
            color[start] = 0;
            q.push(start);
            while (!q.empty())
            {
                size_t curr = q.front(); q.pop();
                for (size_t nb : graph.adjacency_list[curr])
                    if (color.find(nb) == color.end())
                    {
                        color[nb] = 1 - color[curr]; q.push(nb);
                    }
            }
        }

        std::vector<size_t> left, right;
        for (const auto& [id, col] : color)
            if (col == 0) left.push_back(id);
            else right.push_back(id);

        float left_x = 0.15f, right_x = 0.75f;
        float left_step = 0.8f / std::max(1, (int)left.size());
        float right_step = 0.8f / std::max(1, (int)right.size());

        for (size_t i = 0; i < left.size(); ++i)
            graph.nodes[left[i]].position = ImVec2(
                left_x,
                0.1f + i * left_step
            );
        for (size_t i = 0; i < right.size(); ++i)
            graph.nodes[right[i]].position = ImVec2(
                right_x,
                0.1f + i * right_step
            );
    }

    void GraphLayoutEngine::compute_vertical_tree(
        GraphData& graph,
        const ImVec2& bounds)
    {
        compute_hierarchical(graph, bounds);
    }

    void GraphLayoutEngine::compute_horizontal_tree(
        GraphData& graph,
        const ImVec2& bounds)
    {
        compute_hierarchical(graph, bounds);
        for (auto& [id, node] : graph.nodes)
            std::swap(
                node.position.x,
                node.position.y
            );
    }

    void GraphLayoutEngine::compute_random(
        GraphData& graph,
        const ImVec2& bounds)
    {
        std::uniform_real_distribution<float> dist(0.05f, 0.95f);
        for (auto& [id, node] : graph.nodes)
        {
            node.position = ImVec2(dist(m_rng), dist(m_rng));
            node.velocity = ImVec2(0, 0);
        }
    }


} // namespace c2l::algorithms