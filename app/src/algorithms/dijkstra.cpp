#include "algorithms/dijkstra.hpp"
#include "core/utils/logger/logger.hpp"
#include "core/utils/variables.hpp"

#include <algorithm>
#include <sstream>

namespace c2l::algorithms
{
    Dijkstra::Dijkstra(core::JsonConfigManager& json_config_manager)
        : JsonAlgorithmBase(json_config_manager, AlgorithmType::DIJKSTRA)
    {
        LOG_DEBUG("Dijkstra created and metadata loaded from JSON");
    }

    void Dijkstra::set_graph_structure(
        const std::vector<std::vector<size_t>>& adjacency_list)
    {
        // Convert unweighted graph to weighted with weight 1
        m_graph.adjacency_list.clear();
        m_graph.adjacency_list.resize(adjacency_list.size());

        for (size_t i = 0; i < adjacency_list.size(); ++i)
        {
            for (size_t neighbor : adjacency_list[i])
            {
                m_graph.adjacency_list[i].emplace_back(neighbor, 1);
            }
        }

        m_graph.node_count = adjacency_list.size();
        m_graph.edge_count = 0;

        for (const auto& neighbors : m_graph.adjacency_list)
        {
            m_graph.edge_count += neighbors.size();
        }

        if (!m_graph.is_directed)
        {
            m_graph.edge_count /= 2;
        }
    }

    void Dijkstra::set_graph_structure_weighted(
        const std::vector<std::vector<std::pair<size_t, int>>>& weighted_adjacency_list)
    {
        m_graph.adjacency_list.clear();
        m_graph.adjacency_list.resize(weighted_adjacency_list.size());

        for (size_t i = 0; i < weighted_adjacency_list.size(); ++i)
        {
            for (const auto& [neighbor, weight] : weighted_adjacency_list[i])
            {
                m_graph.adjacency_list[i].emplace_back(neighbor, weight);
            }
        }

        m_graph.node_count = weighted_adjacency_list.size();
        m_graph.edge_count = 0;

        for (const auto& neighbors : m_graph.adjacency_list)
        {
            m_graph.edge_count += neighbors.size();
        }

        if (!m_graph.is_directed)
        {
            m_graph.edge_count /= 2;
        }
    }

    void Dijkstra::set_start_node(size_t start)
    {
        if (start < m_graph.node_count)
        {
            m_start_node.value() = start;
        }
        else
        {
            LOG_WARNING(
                "Invalid start node: {}, graph has {} nodes", 
                start, 
                m_graph.node_count
            );
        }
    }

    void Dijkstra::set_target_node(std::optional<size_t> target)
    {
        m_target_node = target;
        reset();
        generate_all_steps();
    }

    void Dijkstra::generate_all_steps()
    {
        if (m_graph.node_count == 0)
        {
            LOG_WARNING("Cannot generate Dijkstra steps: graph has no nodes");
            return;
        }

        if (m_start_node.value() >= m_graph.node_count)
        {
            LOG_ERROR("Invalid start node: {}", m_start_node.value());
            return;
        }

        m_steps.clear();

        // Initialize state
        DijkstraState state;
        state.settled.resize(m_graph.node_count, false);
        state.in_pq.resize(m_graph.node_count, false);
        state.previous.resize(m_graph.node_count, static_cast<size_t>(-1));
        state.distance.resize(m_graph.node_count, static_cast<int>(core::INF));
        state.phase = DijkstraState::Phase::INITIALIZE;
        state.explored_count = 0;
        state.comparisons = 0;
        state.is_complete = false;
        state.target_found = false;
        state.current_node = static_cast<size_t>(-1);
        state.current_neighbor = static_cast<size_t>(-1);
        state.current_edge_weight = 0;
        state.new_distance = 0;
        state.old_distance = 0;
        state.update_occurred = false;

        // Initialization
        push_step(state, "init");

        // Pushing source node
        state.distance[m_start_node.value()] = 0;
        state.pq.emplace(0, m_start_node.value());
        state.in_pq[m_start_node.value()] = true;
        state.phase = DijkstraState::Phase::PUSH_SOURCE;
        push_step(state, "push_source");

        // Process Dijkstra until complete or target found
        while (!state.is_complete && 
               !state.target_found && 
               !state.pq.empty())
        {
            // Extract minimum
            auto [current_dist, current_node] = state.pq.top();
            state.pq.pop();
            state.in_pq[current_node] = false;
            state.current_node = current_node;
            state.phase = DijkstraState::Phase::EXTRACT_MIN;
            push_step(state, "extract_min");

            // Skip if already settled (stale entry)
            if (state.settled[current_node])
            {
                continue;
            }

            // Settle node
            state.settled[current_node] = true;
            state.explored_count++;
            state.traversal_order.push_back(current_node);
            state.phase = DijkstraState::Phase::SETTLE_NODE;
            push_step(state, "settle_node");

            // Check if target found
            if (m_target_node.has_value() && 
                current_node == m_target_node.value())
            {
                state.target_found = true;
                state.target_node_found = current_node;
                state.phase = DijkstraState::Phase::TARGET_FOUND;
                push_step(state, "target_found");
                break;
            }

            // Relax all edges from current node
            const auto& edges = m_graph.adjacency_list[current_node];

            for (size_t i = 0; i < edges.size(); ++i)
            {
                const Edge& edge = edges[i];
                size_t neighbor = edge.to;
                int weight = edge.weight;

                state.current_neighbor_index = i;
                state.current_neighbor = neighbor;
                state.current_edge_weight = weight;
                state.comparisons++;

                // Relax edge step
                state.phase = DijkstraState::Phase::RELAX_EDGE;
                push_step(state, "relax_edge");

                int new_dist = state.distance[current_node] + weight;
                state.new_distance = new_dist;
                state.old_distance = state.distance[neighbor];

                if (new_dist < state.distance[neighbor])
                {
                    state.update_occurred = true;
                    state.phase = DijkstraState::Phase::UPDATE_DISTANCE;
                    push_step(state, "update_distance");

                    // Update distance and push to priority queue
                    state.distance[neighbor] = new_dist;
                    state.previous[neighbor] = current_node;
                    state.pq.emplace(new_dist, neighbor);
                    state.in_pq[neighbor] = true;

                    state.phase = DijkstraState::Phase::PUSH_NEIGHBOR;
                    push_step(state, "push_neighbor");
                }
                else
                {
                    state.update_occurred = false;
                }
            }

            state.current_neighbor = static_cast<size_t>(-1);
            state.current_neighbor_index = 0;
            state.update_occurred = false;
            state.phase = DijkstraState::Phase::NODE_SETTLED;
            push_step(state, "node_settled");
        }

        // Final steps
        if (state.pq.empty() && !state.target_found)
        {
            state.is_complete = true;
            state.phase = DijkstraState::Phase::PQ_EMPTY;
            push_step(state, "pq_empty");
        }

        state.phase = DijkstraState::Phase::COMPLETED;
        push_step(state, "completed");

        LOG_DEBUG("Generated {} steps for Dijkstra", m_steps.size());
    }

    bool Dijkstra::perform_dijkstra_step(DijkstraState& state)
    {
        // This method is kept for potential single-step execution
        return true;
    }

    void Dijkstra::relax_edge(
        DijkstraState& state, 
        size_t neighbor, 
        int weight)
    {
        // Handled in generate_all_steps
    }

    void Dijkstra::push_step(
        const DijkstraState& state, 
        const std::string& operation_id)
    {
        auto step = create_step_from_state(state, operation_id);
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep Dijkstra::create_step_from_state(
        const DijkstraState& state, 
        const std::string& operation_id
    ) const
    {
        AlgorithmStep step;

        // Use node values as data
        if (!m_graph.node_weights.empty())
        {
            step.data = m_graph.node_weights;
        }
        else
        {
            for (size_t i = 0; i < m_graph.node_count; ++i)
            {
                step.data.push_back(static_cast<int>(i));
            }
        }

        step.metadata.operation_id = operation_id;
        populate_step_metadata(
            step, 
            state, 
            operation_id
        );
        step.description = format_step_description(
            operation_id, 
            step
        );
        update_visualization_data(
            step, 
            state, 
            operation_id
        );

        return step;
    }

    void Dijkstra::populate_step_metadata(
        AlgorithmStep& step, 
        const DijkstraState& state, 
        const std::string& operation_id
    ) const
    {
        step.metadata.set(
            "node_count", 
            m_graph.node_count, 
            "Total nodes in graph"
        );
        step.metadata.set(
            "edge_count", 
            m_graph.edge_count, 
            "Total edges in graph"
        );
        step.metadata.set(
            "start_node", 
            m_start_node.value(), 
            "Source node for Dijkstra"
        );
        step.metadata.set(
            "current_node", 
            state.current_node, 
            "Node currently being processed"
        );
        step.metadata.set(
            "explored_count", 
            state.explored_count, 
            "Number of nodes settled"
        );
        step.metadata.set(
            "comparisons", 
            state.comparisons, 
            "Number of edge relaxations"
        );
        step.metadata.set(
            "pq_size", 
            state.pq.size(), 
            "Current priority queue size"
        );

        if (m_target_node.has_value())
        {
            step.metadata.set(
                "target_node", 
                m_target_node.value(), 
                "Target node being searched for"
            );
        }

        if (state.current_node < state.distance.size() && 
            state.current_node != static_cast<size_t>(-1))
        {
            if (std::isfinite(state.distance[state.current_node]))
            {
                step.metadata.set(
                    "distance", 
                    state.distance[state.current_node],
                    "Current shortest distance from source"
                );
            }
        }



        if (state.current_neighbor != static_cast<size_t>(-1))
        {
            step.metadata.set(
                "explored_neighbor", 
                state.current_neighbor,
                "Currently exploring neighbor"
            );
            step.metadata.set(
                "edge_weight", 
                state.current_edge_weight,
                "Weight of edge being explored"
            );

            if (std::isfinite(state.new_distance))
            {
                step.metadata.set(
                    "new_distance", 
                    state.new_distance,
                    "Potential new distance to neighbor"
                );
            }
            
            if (std::isfinite(state.old_distance))
            {
                step.metadata.set(
                    "old_distance",
                    std::string("INFINIT"),
                    "Current distance to neighbor"
                );
            }
            else
            {
                step.metadata.set(
                    "old_distance",
                    state.old_distance,
                    "Current distance to neighbor"
                );
            }
        }

        if (state.target_found)
        {
            step.metadata.set(
                "found_at_distance", 
                state.distance[state.target_node_found],
                "Distance at which target was found"
            );
            step.metadata.set(
                "found_at_node", 
                state.target_node_found,
                "Target node found"
            );
        }

        if (operation_id == "init" || operation_id == "push_source")
        {
            step.metadata.set(
                "source_node", 
                m_start_node.value(),
                "source_node/start node");
        }

        // Phase as string
        std::string phase_str;
        switch (state.phase)
        {
            case DijkstraState::Phase::INITIALIZE: 
                phase_str = "Initializing"; break;
            case DijkstraState::Phase::PUSH_SOURCE: 
                phase_str = "Pushing Source Node"; break;
            case DijkstraState::Phase::EXTRACT_MIN: 
                phase_str = "Extracting Minimum"; break;
            case DijkstraState::Phase::SETTLE_NODE: 
                phase_str = "Settling Node"; break;
            case DijkstraState::Phase::RELAX_EDGE: 
                phase_str = "Relaxing Edge"; break;
            case DijkstraState::Phase::UPDATE_DISTANCE: 
                phase_str = "Updating Distance"; break;
            case DijkstraState::Phase::PUSH_NEIGHBOR: 
                phase_str = "Pushing Neighbor"; break;
            case DijkstraState::Phase::NODE_SETTLED: 
                phase_str = "Node Settled"; break;
            case DijkstraState::Phase::PQ_EMPTY: 
                phase_str = "Priority Queue Empty"; break;
            case DijkstraState::Phase::TARGET_FOUND: 
                phase_str = "Target Found"; break;
            case DijkstraState::Phase::COMPLETED: 
                phase_str = "Completed"; break;
        }
        step.metadata.set(
            "phase", 
            phase_str, 
            "Current Dijkstra phase"
        );
    }

    void Dijkstra::update_visualization_data(
        AlgorithmStep& step,
        const DijkstraState& state,
        const std::string& operation_id
    ) const
    {
        auto& viz = step.visualization;

        // Clear previous state
        viz.graph_state.visited_nodes.clear();
        viz.graph_state.frontier_nodes.clear();
        viz.graph_state.active_edges.clear();
        viz.graph_state.path.clear();
        viz.graph_state.node_distances.clear();
        viz.graph_state.node_parents.clear();
        viz.highlighted_index.reset();
        viz.compared_index.reset();
        viz.additional_highlights.clear();

        // Set settled nodes (visited) and distances
        for (size_t i = 0; i < state.settled.size(); ++i)
        {
            if (state.settled[i])
            {
                viz.graph_state.visited_nodes.push_back(i);
                if (std::isfinite(state.distance[i]))
                {
                    viz.graph_state.node_distances[i] = state.distance[i];
                }
                if (state.previous[i] != static_cast<size_t>(-1))
                {
                    viz.graph_state.node_parents[i] = state.previous[i];
                }
            }
        }

        // Set frontier nodes (in priority queue but not settled)
        // Need to extract from pq (which is a priority_queue, can't iterate directly)
        // We'll use the in_pq array to track frontier nodes
        for (size_t i = 0; i < state.in_pq.size(); ++i)
        {
            if (state.in_pq[i] && !state.settled[i])
            {
                viz.graph_state.frontier_nodes.push_back(i);
            }
        }

        // Set active edge
        if (state.phase == DijkstraState::Phase::RELAX_EDGE &&
            state.current_node != static_cast<size_t>(-1) &&
            state.current_neighbor != static_cast<size_t>(-1))
        {
            viz.graph_state.active_edges.emplace_back(
                state.current_node, 
                state.current_neighbor
            );
        }

        // Building path if target found
        if (state.target_found && 
            state.target_node_found < state.previous.size())
        {
            std::vector<size_t> path;
            size_t current = state.target_node_found;
            while (current != static_cast<size_t>(-1))
            {
                path.push_back(current);
                current = state.previous[current];
            }
            std::reverse(path.begin(), path.end());
            viz.graph_state.path = path;
        }

        // Setting step-specific highlights
        if (operation_id == "settle_node" && 
            state.current_node != static_cast<size_t>(-1))
        {
            viz.highlighted_index = state.current_node;
        }
        else if (operation_id == "relax_edge" && 
                 state.current_neighbor != static_cast<size_t>(-1))
        {
            viz.compared_index = state.current_node;
            viz.additional_highlights.push_back(state.current_neighbor);
        }
        else if (operation_id == "update_distance" && 
                 state.current_neighbor != static_cast<size_t>(-1))
        {
            viz.additional_highlights.push_back(state.current_neighbor);
        }
        else if (operation_id == "push_neighbor" && 
                 state.current_neighbor != static_cast<size_t>(-1))
        {
            viz.additional_highlights.push_back(state.current_neighbor);
        }
        else if (operation_id == "extract_min" && 
                 state.current_node != static_cast<size_t>(-1))
        {
            viz.highlighted_index = state.current_node;
        }

        // Set metrics
        viz.comparison_count = state.comparisons;
        viz.swap_count = 0;
        viz.is_complete = state.is_complete || state.target_found;
    }

    void Dijkstra::reset_state()
    {
        m_current_state = DijkstraState{};
        m_current_state.settled.resize(
            m_graph.node_count, 
            false
        );
        m_current_state.in_pq.resize(
            m_graph.node_count, 
            false
        );
        m_current_state.previous.resize(
            m_graph.node_count, 
            static_cast<size_t>(-1)
        );
        m_current_state.distance.resize(
            m_graph.node_count, 
            static_cast<int>(core::INF)
        );
    }

} // namespace c2l::algorithms