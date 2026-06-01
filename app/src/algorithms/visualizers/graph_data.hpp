#ifndef CODE2LOGIC_GRAPH_DATA_HPP
#define CODE2LOGIC_GRAPH_DATA_HPP

#include <vector>
#include <unordered_map>
#include <string>
#include <optional>

#include "imgui.h"


namespace c2l::algorithms
{
        struct GraphNode
    {
        size_t id{0};
        std::string label;
        int value{0};
        ImVec2 position{0.9f, 0.9f};
        ImVec2 velocity{0, 0};
        bool is_fixed{false};
        ImU32 custom_color{0};

        // Algorithm state
        bool visited{false};
        bool in_queue{false};
        int distance{-1};
        size_t parent{static_cast<size_t>(-1)};

        GraphNode() = default;
        GraphNode(
            size_t id,
            const std::string& label = "",
            int val = 0)
            : id(id)
            , label(label.empty() ? std::to_string(id) : label)
            , value(val)
        {}
    };

    struct GraphEdge
    {
        size_t from;
        size_t to;
        int weight{1};
        bool is_directed{false};

        GraphEdge() = default;
        GraphEdge(size_t from, size_t to, int weight = 1, bool directed = false)
            : from(from), to(to), weight(weight), is_directed(directed) {}

        bool operator==(const GraphEdge& other) const {
            return (from == other.from && to == other.to);
        }
    };

    struct GraphData
    {
        std::unordered_map<size_t, GraphNode> nodes;
        std::vector<GraphEdge> edges;
        std::unordered_map<size_t, std::vector<size_t>> adjacency_list;
        size_t next_node_id{0};
        bool is_directed{false};
        bool is_weighted{false};
        size_t start_node{0};
        std::optional<size_t> target_node;

        size_t add_node(const std::string& label = "", int value = 0);
        void remove_node(size_t id);
        bool add_edge(size_t from, size_t to, int weight = 1);
        void remove_edge(size_t from, size_t to);
        bool has_node(size_t id) const;
        bool has_edge(size_t from, size_t to) const;
        size_t node_count() const { return nodes.size(); }
        size_t edge_count() const { return edges.size(); }
        void clear();
        std::vector<std::vector<size_t>> get_adjacency_matrix() const;
        std::vector<int> get_node_values() const;
    };
}

#endif