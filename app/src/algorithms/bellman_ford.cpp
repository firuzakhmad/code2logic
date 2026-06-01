#include "algorithms/bellman_ford.hpp"
#include "core/utils/logger/logger.hpp"

#include <algorithm>
#include <sstream>
#include <queue>

#include "core/utils/variables.hpp"

namespace c2l::algorithms
{
    BellmanFord::BellmanFord(
        core::JsonConfigManager& json_config_manager)
        : JsonAlgorithmBase(json_config_manager, AlgorithmType::BELLMAN_FORD)
    {
        LOG_DEBUG("Bellman-Ford created and metadata loaded from JSON");
    }

    void BellmanFord::set_graph_structure(
        const std::vector<std::vector<size_t>>& adjacency_list)
    {
        // Converting unweighted graph to weighted with weight 1
        m_graph.edges.clear();
        m_graph.adjacency_list.clear();
        m_graph.adjacency_list.resize(adjacency_list.size());

        for (size_t i = 0; i < adjacency_list.size(); ++i)
        {
            for (size_t neighbor : adjacency_list[i])
            {
                m_graph.edges.emplace_back(i, neighbor, 1);
                m_graph.adjacency_list[i].emplace_back(neighbor, 1);
            }
        }

        // For undirected graphs, add reverse edges
        if (!m_graph.is_directed)
        {
            size_t original_size = m_graph.edges.size();
            for (size_t i = 0; i < original_size; ++i)
            {
                const auto& edge = m_graph.edges[i];
                m_graph.edges.emplace_back(edge.to, edge.from, edge.weight);
                m_graph.adjacency_list[edge.to].emplace_back(edge.from, edge.weight);
            }
        }

        m_graph.node_count = adjacency_list.size();
        m_graph.edge_count = m_graph.edges.size();
    }

    void BellmanFord::set_graph_structure_weighted(
        const std::vector<std::vector<std::pair<size_t, int>>>& weighted_adjacency_list)
    {
        m_graph.edges.clear();
        m_graph.adjacency_list = weighted_adjacency_list;
        m_graph.is_directed = true;  // Default to directed for weighted

        for (size_t i = 0; i < weighted_adjacency_list.size(); ++i)
        {
            for (const auto& [neighbor, weight] : weighted_adjacency_list[i])
            {
                m_graph.edges.emplace_back(i, neighbor, weight);
            }
        }

        m_graph.node_count = weighted_adjacency_list.size();
        m_graph.edge_count = m_graph.edges.size();
    }

    void BellmanFord::set_start_node(size_t start)
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

    void BellmanFord::set_target_node(
        std::optional<size_t> target)
    {
        m_target_node = target;
        reset();
        generate_all_steps();
    }

    void BellmanFord::generate_all_steps()
    {
        if (m_graph.node_count == 0)
        {
            LOG_WARNING("Cannot generate Bellman-Ford steps: graph has no nodes");
            return;
        }

        if (m_start_node.value() >= m_graph.node_count)
        {
            LOG_ERROR("Invalid start node: {}", m_start_node.value());
            return;
        }

        m_steps.clear();

        // Initialization state
        BellmanFordState state;
        state.distance.resize(m_graph.node_count, core::INF);
        state.previous.resize(m_graph.node_count, static_cast<size_t>(-1));
        state.reached.resize(m_graph.node_count, false);
        state.phase = BellmanFordState::Phase::INITIALIZE;
        state.explored_count = 0;
        state.comparisons = 0;
        state.is_complete = false;
        state.target_found = false;
        state.negative_cycle_detected = false;
        state.negative_cycle_checked = false;
        state.current_node = static_cast<size_t>(-1);
        state.current_neighbor = static_cast<size_t>(-1);
        state.current_edge_weight = 0;
        state.current_iteration = 0;
        state.updates_in_iteration = 0;
        state.updates_occurred = false;

        // Step 1: Initialization
        push_step(state, "init");

        // Step 2: Set source distance
        state.distance[m_start_node.value()] = 0;
        state.reached[m_start_node.value()] = true;
        state.explored_count = 1;
        push_step(state, "init"); // Re-push to show distance update

        size_t max_iterations = m_graph.node_count - 1;

        // Run V-1 iterations of edge relaxation
        for (state.current_iteration = 0; 
             state.current_iteration < max_iterations; 
             ++state.current_iteration)
        {
            if (state.target_found || state.is_complete) break;

            state.updates_in_iteration = 0;
            state.updates_occurred = false;
            state.phase = BellmanFordState::Phase::ITERATION_START;
            push_step(state, "iteration_start");

            // Relax all edges
            for (size_t edge_idx = 0; edge_idx < m_graph.edges.size(); ++edge_idx)
            {
                const auto& edge = m_graph.edges[edge_idx];
                state.current_edge_index = edge_idx;
                state.current_node = edge.from;
                state.current_neighbor = edge.to;
                state.current_edge_weight = edge.weight;
                state.old_distance = state.distance[edge.to];

                state.phase = BellmanFordState::Phase::RELAX_EDGE;
                push_step(state, "relax_edge");
                state.comparisons++;

                // Check if we can relax this edge
                if (state.distance[edge.from] < core::INF &&
                    state.distance[edge.from] + edge.weight < state.distance[edge.to])
                {
                    state.new_distance = state.distance[edge.from] + edge.weight;
                    state.distance[edge.to] = state.new_distance;
                    state.previous[edge.to] = edge.from;
                    if (!state.reached[edge.to])
                    {
                        state.reached[edge.to] = true;
                        state.explored_count++;
                    }

                    state.visited_count++;
                    state.updates_in_iteration++;
                    state.updates_occurred = true;

                    state.phase = BellmanFordState::Phase::UPDATE_DISTANCE;
                    push_step(state, "update_distance");

                    // Check if target found
                    if (m_target_node.has_value() && edge.to == m_target_node.value())
                    {
                        state.target_found = true;
                        state.target_node_found = edge.to;
                        state.phase = BellmanFordState::Phase::TARGET_FOUND;
                        push_step(state, "target_found");
                        break;
                    }
                }
            }

            if (state.target_found) break;

            state.phase = BellmanFordState::Phase::ITERATION_END;
            push_step(state, "iteration_end");

            // Early termination if no updates occurred
            if (!state.updates_occurred)
            {
                state.phase = BellmanFordState::Phase::NO_UPDATES;
                push_step(state, "no_updates");
                break;
            }
        }

        // Negative cycle detection (only if no target found yet)
        if (!state.target_found && !state.negative_cycle_checked)
        {
            state.phase = BellmanFordState::Phase::NEGATIVE_CYCLE_CHECK;
            push_step(state, "negative_cycle_check");

            for (const auto& edge : m_graph.edges)
            {
                if (state.distance[edge.from] < core::INF &&
                    state.distance[edge.from] + edge.weight < state.distance[edge.to])
                {
                    state.negative_cycle_detected = true;
                    state.phase = BellmanFordState::Phase::NEGATIVE_CYCLE_DETECTED;
                    push_step(state, "negative_cycle");
                    break;
                }
            }
            state.negative_cycle_checked = true;
        }

        // Count reached nodes (nodes with finite distance)
        for (size_t i = 0; i < state.distance.size(); ++i)
        {
            if (state.distance[i] < core::INF)
            {
                state.explored_count++;
            }
        }

        state.is_complete = true;
        state.phase = BellmanFordState::Phase::COMPLETED;
        push_step(state, "completed");

        LOG_DEBUG("Generated {} steps for Bellman-Ford", m_steps.size());
    }

    void BellmanFord::push_step(
        const BellmanFordState& state, 
        const std::string& operation_id)
    {
        auto step = create_step_from_state(state, operation_id);
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep BellmanFord::create_step_from_state(
        const BellmanFordState& state, 
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
        populate_step_metadata(step, state, operation_id);
        step.description = format_step_description(operation_id, step);
        update_visualization_data(step, state, operation_id);

        return step;
    }

    void BellmanFord::populate_step_metadata(
        AlgorithmStep& step, 
        const BellmanFordState& state, 
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
            "source_node", 
            m_start_node.value(), 
            "Source node for Bellman-Ford"
        );
        step.metadata.set(
            "current_iteration", 
            state.current_iteration, 
            "Current relaxation iteration"
        );
        step.metadata.set(
            "max_iterations", 
            m_graph.node_count - 1, 
            "Maximum iterations (V-1)"
        );
        step.metadata.set(
            "explored_count", 
            state.explored_count, 
            "Number of nodes with finite distance"
        );
        step.metadata.set(
            "comparisons", 
            state.comparisons, 
            "Number of edge relaxations"
        );
        step.metadata.set(
            "updates_in_iteration", 
            state.updates_in_iteration, 
            "Distance updates in current iteration"
        );

        if (m_target_node.has_value())
        {
            step.metadata.set(
                "target_node", 
                m_target_node.value(), 
                "Target node being searched for"
            );
        }

        if (state.current_node != static_cast<size_t>(-1))
        {
            step.metadata.set(
                "current_node", 
                state.current_node, 
                "From node of edge being relaxed"
            );
            
            if (state.current_node < state.distance.size())
            {
                if (state.distance[state.current_node] < core::INF)
                {
                    step.metadata.set(
                        "current_distance", 
                        state.distance[state.current_node],
                        "Current distance to from node"
                    );
                }
            }
        }

        if (state.current_neighbor != static_cast<size_t>(-1))
        {
            step.metadata.set(
                "explored_neighbor", 
                state.current_neighbor,
                "To node of edge being relaxed"
            );
            step.metadata.set(
                "edge_weight", 
                state.current_edge_weight,
                "Weight of edge being relaxed"
            );
            step.metadata.set(
                "distance",
                state.distance[state.current_neighbor],
                "Current shortest distance to neighbor"
            );

            if (state.old_distance < core::INF)
            {
                step.metadata.set(
                    "old_distance", 
                    state.old_distance,
                    "Current distance to neighbor"
                );
            } 
            else 
            {
                step.metadata.set(
                    "old_distance", 
                    "INFINITY",
                    "Current distance to neighbor"
                );
            }
            if (state.new_distance < core::INF)
            {
                step.metadata.set(
                    "new_distance", 
                    state.new_distance,
                    "New distance to neighbor after relaxation"
                );
            }
            else
            {
                step.metadata.set(
                    "new_distance", 
                    "INFINITY",
                    "New distance to neighbor after relaxation"
                );
            }
        }

        if (state.target_found)
        {
            step.metadata.set(
                "found_at_distance", 
                state.distance[state.target_node_found],
                "Shortest distance to target"
            );
            step.metadata.set(
                "found_at_node", 
                state.target_node_found,
                "Target node found"
            );
            step.metadata.set(
                "distance",
                state.distance[state.current_neighbor],
                "Current shortest distance to neighbor"
            );
        }

        if (state.negative_cycle_detected)
        {
            step.metadata.set(
                "negative_cycle_detected", 
                true,
                "Graph contains a negative cycle"
            );
        }

        // Phase as string
        std::string phase_str;
        switch (state.phase)
        {
            case BellmanFordState::Phase::INITIALIZE: 
                phase_str = "Initializing"; break;
            case BellmanFordState::Phase::ITERATION_START: 
                phase_str = "Starting Relaxation Iteration"; break;
            case BellmanFordState::Phase::RELAX_EDGE: 
                phase_str = "Relaxing Edge"; break;
            case BellmanFordState::Phase::UPDATE_DISTANCE: 
                phase_str = "Updating Distance"; break;
            case BellmanFordState::Phase::ITERATION_END: 
                phase_str = "Completing Iteration"; break;
            case BellmanFordState::Phase::NO_UPDATES: 
                phase_str = "No Updates - Early Termination"; break;
            case BellmanFordState::Phase::NEGATIVE_CYCLE_CHECK: 
                phase_str = "Checking for Negative Cycles"; break;
            case BellmanFordState::Phase::NEGATIVE_CYCLE_DETECTED: 
                phase_str = "Negative Cycle Detected!"; break;
            case BellmanFordState::Phase::TARGET_FOUND: 
                phase_str = "Target Found!"; break;
            case BellmanFordState::Phase::COMPLETED: 
                phase_str = "Completed"; break;
        }
        step.metadata.set(
            "phase", 
            phase_str, 
            "Current Bellman-Ford phase"
        );
    }

    void BellmanFord::update_visualization_data(
        AlgorithmStep& step, 
        const BellmanFordState& state, 
        const std::string& operation_id
    ) const
    {
        auto& viz = step.visualization;

        // Clearing previous state
        viz.graph_state.visited_nodes.clear();
        viz.graph_state.frontier_nodes.clear();
        viz.graph_state.active_edges.clear();
        viz.graph_state.path.clear();
        viz.graph_state.node_distances.clear();
        viz.graph_state.node_parents.clear();
        viz.highlighted_index.reset();
        viz.compared_index.reset();
        viz.additional_highlights.clear();

        // Setting nodes with finite distance as "visited"
        for (size_t i = 0; i < state.distance.size(); ++i)
        {
            if (state.distance[i] < core::INF)
            {
                viz.graph_state.visited_nodes.push_back(i);
                viz.graph_state.node_distances[i] = state.distance[i];
            }
            if (state.previous[i] != static_cast<size_t>(-1))
            {
                viz.graph_state.node_parents[i] = state.previous[i];
            }
        }

        // Setting active edge
        if ((operation_id == "relax_edge" || 
            operation_id == "update_distance") &&
            state.current_node != static_cast<size_t>(-1) &&
            state.current_neighbor != static_cast<size_t>(-1))
        {
            viz.graph_state.active_edges.emplace_back(
                state.current_node, 
                state.current_neighbor
            );
        }

        // Build path if target found
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

        // Set step-specific highlights
        if ((operation_id == "update_distance") && 
            state.current_neighbor != static_cast<size_t>(-1))
        {
            viz.additional_highlights.push_back(state.current_neighbor);
        }
        else if (operation_id == "relax_edge" && 
            state.current_neighbor != static_cast<size_t>(-1))
        {
            viz.compared_index = state.current_node;
            viz.additional_highlights.push_back(state.current_neighbor);
        }
        else if (state.current_node != static_cast<size_t>(-1))
        {
            viz.highlighted_index = state.current_node;
        }

        // Negative cycle warning in visualization
        if (state.negative_cycle_detected)
        {
            viz.additional_highlights.push_back(
                static_cast<size_t>(-1)
            ); // Special marker for cycle
        }

        // Set metrics
        viz.comparison_count = state.comparisons;
        viz.explored_node_count =
            std::count(
                state.reached.begin(),
                state.reached.end(),
                true
            );

        viz.visited_node_count =
            state.updates_in_iteration;

        viz.swap_count = 0;
        viz.is_complete = 
            state.is_complete || 
            state.target_found || 
            state.negative_cycle_detected;
    }

    void BellmanFord::reset_state()
    {
        m_current_state = BellmanFordState{};
        m_current_state.distance.resize(
            m_graph.node_count, 
            core::INF
        );
        m_current_state.previous.resize(
            m_graph.node_count, 
            static_cast<size_t>(-1)
        );
        m_current_state.reached.resize(
            m_graph.node_count, 
            false
        );
    }

} // namespace c2l::algorithms