#include "algorithms/bfs.hpp"
#include "core/utils/logger/logger.hpp"

#include <algorithm>
#include <sstream>

namespace c2l::algorithms
{
    BFS::BFS(core::JsonConfigManager& json_config_manager)
        : JsonAlgorithmBase(json_config_manager, AlgorithmType::BFS)
    {

        LOG_DEBUG("BFS created and metadata loaded from JSON");
    }

    void BFS::set_graph_structure(
        const std::vector<std::vector<size_t>>& adjacency_list)
    {
        m_graph.adjacency_list = adjacency_list;
        m_graph.node_count = adjacency_list.size();
        m_graph.edge_count = 0;

        for (const auto& neighbors : adjacency_list)
        {
            m_graph.edge_count += neighbors.size();
        }

        if (!m_graph.is_directed)
        {
            m_graph.edge_count /= 2;
        }
    }

    void BFS::set_start_node(size_t start)
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

    void BFS::set_target_node(std::optional<size_t> target)
    {
        m_target_node = target;
        reset();
        generate_all_steps();
    }

    void BFS::generate_all_steps()
    {
        if (m_graph.node_count == 0)
        {
            LOG_WARNING("Cannot generate BFS steps: graph has no nodes");
            return;
        }

        if (m_start_node.value() >= m_graph.node_count)
        {
            LOG_ERROR("Invalid start node: {}", m_start_node.value());
            return;
        }

        m_steps.clear();

        // Initialize state
        BFSState state;
        state.visited.resize(m_graph.node_count, false);
        state.in_queue.resize(m_graph.node_count, false);
        state.parent.resize(m_graph.node_count, static_cast<size_t>(-1));
        state.distance.resize(m_graph.node_count, -1);
        state.phase = BFSState::Phase::INITIALIZE;
        state.current_level = 0;
        state.explored_count = 0;
        state.comparisons = 0;
        state.is_complete = false;
        state.target_found = false;
        state.current_node = static_cast<size_t>(-1);
        state.current_neighbor = static_cast<size_t>(-1);
        state.current_neighbor_index = 0;

        // Initialization
        push_step(state, "init");

        // Enqueue start node
        state.queue.push(m_start_node.value());
        state.in_queue[m_start_node.value()] = true;
        state.distance[m_start_node.value()] = 0;
        state.current_level = 0;
        state.explored_count++;
        state.phase = BFSState::Phase::ENQUEUE_START;
        push_step(state, "enqueue_start");

        // Process BFS until complete or target found
        while (!state.is_complete && !state.target_found && !state.queue.empty())
        {
            // Dequeue node
            state.current_node = state.queue.front();
            state.queue.pop();
            state.in_queue[state.current_node] = false;
            state.phase = BFSState::Phase::DEQUEUE_NODE;
            push_step(state, "dequeue_node");

            // Mark as visited and process node
            if (!state.visited[state.current_node])
            {
                state.visited[state.current_node] = true;
                state.visited_count++;
                state.traversal_order.push_back(state.current_node);
                state.phase = BFSState::Phase::VISIT_NODE;
                push_step(state, "visit_node");

                // Check if target found
                push_step(state, "check_target");

                if (m_target_node.has_value() && 
                    state.current_node == m_target_node.value())
                {
                    state.target_found = true;
                    state.target_node_found = state.current_node;
                    state.phase = BFSState::Phase::TARGET_FOUND;
                    push_step(state, "target_found");
                    break;
                }

                // Explore all neighbors
                const auto& neighbors = m_graph.adjacency_list[state.current_node];

                if (neighbors.empty())
                {
                    state.phase = BFSState::Phase::NODE_PROCESSED;
                    push_step(state, "node_processed");
                    continue;
                }

                for (size_t i = 0; i < neighbors.size(); ++i)
                {
                    size_t neighbor = neighbors[i];
                    state.current_neighbor_index = i;
                    state.current_neighbor = neighbor;
                    state.comparisons++;

                    // Explore edge step
                    state.phase = BFSState::Phase::EXPLORE_EDGE;
                    push_step(state, "explore_edge");

                    if (!state.visited[neighbor] && !state.in_queue[neighbor])
                    {
                        // Enqueue neighbor step
                        add_neighbor_to_queue(state, neighbor);
                    }
                }

                state.current_neighbor = static_cast<size_t>(-1);
                state.current_neighbor_index = 0;
                state.phase = BFSState::Phase::NODE_PROCESSED;
                push_step(state, "node_processed");
            }
        }

        // Final steps
        if (state.queue.empty() && !state.target_found)
        {
            state.is_complete = true;
            state.phase = BFSState::Phase::QUEUE_EMPTY;
            push_step(state, "queue_empty");
        }

        state.phase = BFSState::Phase::COMPLETED;
        push_step(state, "completed");

        LOG_DEBUG("Generated {} steps for BFS", m_steps.size());
    }

    bool BFS::perform_bfs_step(BFSState& state)
    {
        // This method is kept for potential single-step execution
        // The main step generation is done in generate_all_steps()
        return true;
    }

    void BFS::add_neighbor_to_queue(BFSState& state, size_t neighbor)
    {
        state.queue.push(neighbor);
        state.in_queue[neighbor] = true;
        state.explored_count++;

        state.parent[neighbor] = state.current_node;
        state.distance[neighbor] = state.distance[state.current_node] + 1;

        // Update current level if this is a new level
        if (state.distance[neighbor] > static_cast<int>(state.current_level))
        {
            state.current_level = state.distance[neighbor];
        }

        state.phase = BFSState::Phase::ENQUEUE_NEIGHBOR;
        push_step(state, "enqueue_neighbor");
    }

    void BFS::push_step(const BFSState& state, const std::string& operation_id)
    {
        auto step = create_step_from_state(state, operation_id);
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep BFS::create_step_from_state(
        const BFSState& state, 
        const std::string& operation_id
    ) const
    {
        AlgorithmStep step;

        // Using node values as data
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

    void BFS::populate_step_metadata(
        AlgorithmStep& step, 
        const BFSState& state, 
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
            "Starting node for BFS"
        );
        step.metadata.set(
            "current_node", 
            state.current_node, 
            "Node currently being processed"
        );
        step.metadata.set(
            "current_level", 
            state.current_level, 
            "Current BFS level"
        );
        step.metadata.set(
            "explored_count", 
            state.explored_count, 
            "Number of nodes explored"
        );
        step.metadata.set(
            "comparisons", 
            state.comparisons, 
            "Number of adjacency checks"
        );
        step.metadata.set(
            "queue_size", 
            state.queue.size(), 
            "Current queue size"
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
            step.metadata.set(
                "distance", 
                state.distance[state.current_node],
                "Distance from start to current node"
            );
        }

        if (state.current_neighbor != static_cast<size_t>(-1))
        {
            step.metadata.set(
                "explored_neighbor", 
                state.current_neighbor,
                "Currently exploring neighbor"
            );
        }

        if (state.target_found || operation_id == "check_target")
        {
            step.metadata.set(
                "found_at_level", 
                state.distance[state.target_node_found],
                "Level at which target was found"
            );
            step.metadata.set(
                "found_at_node", state.target_node_found,
                "Target node found"
            );
            if (m_target_node.has_value())
            {
                step.metadata.set(
                    "target_node", state.target_node_found,
                    "Target node "
                );
            }
            else
            {
                step.metadata.set(
                    "target_node", "NULL",
                    "Target node "
                );
            }
        }

        // Phase as string
        std::string phase_str;
        switch (state.phase)
        {
            case BFSState::Phase::INITIALIZE: 
                phase_str = "Initializing"; break;
            case BFSState::Phase::ENQUEUE_START: 
                phase_str = "Enqueuing Start Node"; break;
            case BFSState::Phase::DEQUEUE_NODE: 
                phase_str = "Dequeuing Node"; break;
            case BFSState::Phase::VISIT_NODE: 
                phase_str = "Visiting Node"; break;
            case BFSState::Phase::EXPLORE_EDGE: 
                phase_str = "Exploring Edge"; break;
            case BFSState::Phase::ENQUEUE_NEIGHBOR: 
                phase_str = "Enqueuing Neighbor"; break;
            case BFSState::Phase::NODE_PROCESSED: 
                phase_str = "Node Processed"; break;
            case BFSState::Phase::QUEUE_EMPTY: 
                phase_str = "Queue Empty"; break;
            case BFSState::Phase::TARGET_FOUND: 
                phase_str = "Target Found"; break;
            case BFSState::Phase::COMPLETED: 
                phase_str = "Completed"; break;
        }
        step.metadata.set(
            "phase", 
            phase_str, 
            "Current BFS phase"
        );
    }

    void BFS::update_visualization_data(
        AlgorithmStep& step, 
        const BFSState& state, 
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

        // Set visited nodes and distances
        for (size_t i = 0; i < state.visited.size(); ++i)
        {
            if (state.visited[i])
            {
                viz.graph_state.visited_nodes.push_back(i);
                if (state.distance[i] >= 0)
                {
                    viz.graph_state.node_distances[i] = state.distance[i];
                }
                if (state.parent[i] != static_cast<size_t>(-1))
                {
                    viz.graph_state.node_parents[i] = state.parent[i];
                }
            }
        }

        // Set frontier nodes (in queue but not visited)
        std::queue<size_t> temp_queue = state.queue;
        while (!temp_queue.empty())
        {
            size_t node = temp_queue.front();
            temp_queue.pop();
            if (!state.visited[node])
            {
                viz.graph_state.frontier_nodes.push_back(node);
            }
        }

        // Set active edge
        if (state.phase == BFSState::Phase::EXPLORE_EDGE &&
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
            state.target_node_found < state.parent.size())
        {
            std::vector<size_t> path;
            size_t current = state.target_node_found;
            while (current != static_cast<size_t>(-1))
            {
                path.push_back(current);
                current = state.parent[current];
            }
            std::reverse(path.begin(), path.end());
            viz.graph_state.path = path;
        }

        // Set step-specific highlights
        if (operation_id == "visit_node" && 
            state.current_node != static_cast<size_t>(-1))
        {
            viz.highlighted_index = state.current_node;
        }
        else if (operation_id == "explore_edge" && 
            state.current_neighbor != static_cast<size_t>(-1))
        {
            viz.compared_index = state.current_node;
            viz.additional_highlights.push_back(state.current_neighbor);
        }
        else if (operation_id == "enqueue_neighbor" && 
            state.current_neighbor != static_cast<size_t>(-1))
        {
            viz.additional_highlights.push_back(state.current_neighbor);
        }
        else if (operation_id == "dequeue_node" && 
            state.current_node != static_cast<size_t>(-1))
        {
            viz.highlighted_index = state.current_node;
        }

        // Set metrics
        viz.comparison_count = state.comparisons;
        viz.explored_node_count = state.explored_count;
        viz.visited_node_count  = state.visited_count;
        viz.swap_count = 0;
        viz.is_complete = state.is_complete || state.target_found;
    }

    void BFS::reset_state()
    {
        m_current_state = BFSState{};
        m_current_state.visited.resize(
            m_graph.node_count, 
            false
        );
        m_current_state.in_queue.resize(
            m_graph.node_count, 
            false
        );
        m_current_state.parent.resize(
            m_graph.node_count, 
            static_cast<size_t>(-1)
        );
        m_current_state.distance.resize(
            m_graph.node_count, 
            -1
        );
    }

} // namespace c2l::algorithms