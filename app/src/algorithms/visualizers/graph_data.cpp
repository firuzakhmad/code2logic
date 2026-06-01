//
// Created by Akhmad on 5/30/26.
//

#include "algorithms/visualizers/graph_data.hpp"

#include <algorithm>

namespace c2l::algorithms
{
        // GraphData Implementation
    size_t GraphData::add_node(
        const std::string& label,
        int value)
    {
        size_t id = next_node_id++;
        nodes[id] = GraphNode(id, label, value);
        adjacency_list[id] = {};
        return id;
    }

    void GraphData::remove_node(size_t id)
    {
        nodes.erase(id);
        adjacency_list.erase(id);

        edges.erase(
            std::remove_if(
                edges.begin(),
                edges.end(),
                [id](const GraphEdge& e)
                {
                    return e.from == id || e.to == id;
                })
            , edges.end()
        );

        for (auto& [node_id, neighbors] : adjacency_list)
        {
            neighbors.erase(
                std::remove(
                    neighbors.begin(),
                    neighbors.end(), id),
                neighbors.end()
            );
        }
    }

    bool GraphData::add_edge(
        size_t from,
        size_t to,
        int weight)
    {
        if (!has_node(from) || !has_node(to)) return false;
        if (has_edge(from, to)) return false;

        edges.emplace_back(from, to, weight, is_directed);
        adjacency_list[from].push_back(to);
        if (!is_directed) adjacency_list[to].push_back(from);
        return true;
    }

    void GraphData::remove_edge(size_t from, size_t to)
    {
        edges.erase(
            std::remove_if(
                edges.begin(),
                edges.end(),
                [from, to, this](const GraphEdge& e) {
                    return (e.from == from && e.to == to) ||
                           (!is_directed && e.from == to && e.to == from);
                }),
            edges.end()
        );

        auto& neighbors_from = adjacency_list[from];
        neighbors_from.erase(
            std::remove(
                neighbors_from.begin(),
                neighbors_from.end(),
                to),
            neighbors_from.end()
        );

        if (!is_directed)
        {
            auto& neighbors_to = adjacency_list[to];
            neighbors_to.erase(
                std::remove(
                    neighbors_to.begin(),
                    neighbors_to.end(),
                    from),
                neighbors_to.end()
            );
        }
    }

    bool GraphData::has_node(size_t id) const
    {
        return nodes.find(id) != nodes.end();
    }

    bool GraphData::has_edge(size_t from, size_t to) const
    {
        for (const auto& e : edges)
            if ((e.from == from && e.to == to) ||
                (!is_directed && e.from == to && e.to == from))
                return true;
        return false;
    }

    void GraphData::clear()
    {
        nodes.clear();
        edges.clear();
        adjacency_list.clear();
        next_node_id = 0;
        start_node = 0;
        target_node.reset();
    }

    std::vector<std::vector<size_t>> GraphData::get_adjacency_matrix() const
    {
        std::vector<std::vector<size_t>> result(node_count());
        for (const auto& [id, neighbors] : adjacency_list)
        {
            if (id < result.size()) result[id] = neighbors;
        }
        return result;
    }

    std::vector<int> GraphData::get_node_values() const
    {
        std::vector<int> values(node_count(), 0);
        for (const auto& [id, node] : nodes)
            if (id < values.size()) values[id] = node.value;
        return values;
    }

}