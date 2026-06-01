#include "algorithms/dfs.hpp"
#include "core/utils/logger/logger.hpp"

#include <algorithm>
#include <sstream>

namespace c2l::algorithms
{
    DFS::DFS(core::JsonConfigManager& json_config_manager)
        : JsonAlgorithmBase(json_config_manager, AlgorithmType::DFS)
    {
        LOG_DEBUG("DFS created and metadata loaded from JSON");
    }

    void DFS::set_graph_structure(
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

    void DFS::set_start_node(size_t start)
    {
        if (start < m_graph.node_count)
        {
            m_start_node = start;
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

    void DFS::set_target_node(std::optional<size_t> target)
    {
        m_target_node = target;
        reset();
        generate_all_steps();
    }

    void DFS::generate_all_steps()
    {
        if (m_graph.node_count == 0)
        {
            LOG_WARNING("Cannot generate DFS steps: graph has no nodes");
            return;
        }

        if (m_start_node >= m_graph.node_count)
        {
            LOG_ERROR("Invalid start node: {}", m_start_node);
            return;
        }

        m_steps.clear();

        // Initialize state
        DFSState state;
        state.visited.resize(m_graph.node_count, false);
        state.in_stack.resize(m_graph.node_count, false);
        state.parent.resize(m_graph.node_count, static_cast<size_t>(-1));
        state.distance.resize(m_graph.node_count, -1);
        state.entry_time.resize(m_graph.node_count, -1);
        state.exit_time.resize(m_graph.node_count, -1);
        state.phase = DFSState::Phase::INITIALIZE;
        state.current_depth = 0;
        state.explored_count = 0;
        state.comparisons = 0;
        state.time_counter = 0;
        state.is_complete = false;
        state.target_found = false;
        state.current_node = static_cast<size_t>(-1);
        state.current_neighbor = static_cast<size_t>(-1);
        state.current_neighbor_index = 0;
        state.is_backtracking = false;

        // Step 1: Initialization
        push_step(state, "init");

        // Step 2: Push start node
        state.stack.push(m_start_node);
        state.in_stack[m_start_node] = true;
        state.distance[m_start_node] = 0;
        state.current_depth = 0;
        state.phase = DFSState::Phase::PUSH_START;
        push_step(state, "push_start");

        // Process DFS until complete or target found
        while (!state.is_complete && 
               !state.target_found && 
               !state.stack.empty())
        {
            // Pop node
            state.current_node = state.stack.top();
            state.stack.pop();
            state.in_stack[state.current_node] = false;
            state.phase = DFSState::Phase::POP_NODE;
            push_step(state, "pop_node");

            // Mark as visited and process node
            if (!state.visited[state.current_node])
            {
                state.visited[state.current_node] = true;
                state.explored_count++;
                state.traversal_order.push_back(state.current_node);
                state.entry_time[state.current_node] = state.time_counter++;
                state.phase = DFSState::Phase::VISIT_NODE;
                push_step(state, "visit_node");

                // Check if target found
                if (m_target_node.has_value() && 
                    state.current_node == m_target_node.value())
                {
                    state.target_found = true;
                    state.target_node_found = state.current_node;
                    state.phase = DFSState::Phase::TARGET_FOUND;
                    push_step(state, "target_found");
                    break;
                }

                // Explore all neighbors
                const auto& neighbors = m_graph.adjacency_list[state.current_node];

                if (neighbors.empty())
                {
                    // No neighbors - backtrack
                    state.exit_time[state.current_node] = state.time_counter++;
                    state.is_backtracking = true;
                    state.phase = DFSState::Phase::BACKTRACK;
                    push_step(state, "backtrack");
                    state.is_backtracking = false;
                    state.phase = DFSState::Phase::NODE_PROCESSED;
                    push_step(state, "node_processed");
                    continue;
                }

                // Push neighbors onto stack in reverse order to maintain natural DFS order
                // This ensures we process neighbors in the order they appear in adjacency list
                for (auto it = neighbors.rbegin(); it != neighbors.rend(); ++it)
                {
                    size_t neighbor = *it;
                    state.comparisons++;

                    // Explore edge step
                    state.current_neighbor = neighbor;
                    state.phase = DFSState::Phase::EXPLORE_EDGE;
                    push_step(state, "explore_edge");

                    if (!state.visited[neighbor] && !state.in_stack[neighbor])
                    {
                        // Push neighbor step
                        add_neighbor_to_stack(state, neighbor);
                    }
                }
                state.current_neighbor = static_cast<size_t>(-1);

                // Record exit time for the node
                state.exit_time[state.current_node] = state.time_counter++;
                state.is_backtracking = true;
                state.phase = DFSState::Phase::BACKTRACK;
                push_step(state, "backtrack");
                state.is_backtracking = false;
                state.phase = DFSState::Phase::NODE_PROCESSED;
                push_step(state, "node_processed");
            }
        }

        // Final steps
        if (state.stack.empty() && !state.target_found)
        {
            state.is_complete = true;
            state.phase = DFSState::Phase::STACK_EMPTY;
            push_step(state, "stack_empty");
        }

        state.phase = DFSState::Phase::COMPLETED;
        push_step(state, "completed");

        LOG_DEBUG("Generated {} steps for DFS", m_steps.size());
    }

    bool DFS::perform_dfs_step(DFSState& state)
    {
        // This method is kept for potential single-step execution
        // The main step generation is done in generate_all_steps()
        return true;
    }

    void DFS::add_neighbor_to_stack(
        DFSState& state, 
        size_t neighbor)
    {
        state.stack.push(neighbor);
        state.in_stack[neighbor] = true;
        state.parent[neighbor] = state.current_node;
        state.distance[neighbor] = state.distance[state.current_node] + 1;

        // Update current depth
        if (static_cast<int>(state.current_depth) < state.distance[neighbor])
        {
            state.current_depth = state.distance[neighbor];
        }

        state.phase = DFSState::Phase::PUSH_NEIGHBOR;
        push_step(state, "push_neighbor");
    }

    void DFS::push_step(
        const DFSState& state, 
        const std::string& operation_id)
    {
        auto step = create_step_from_state(state, operation_id);
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep DFS::create_step_from_state(
        const DFSState& state, 
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

    void DFS::populate_step_metadata(
        AlgorithmStep& step, 
        const DFSState& state, 
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
            m_start_node, 
            "Starting node for DFS"
        );
        step.metadata.set(
            "current_node", 
            state.current_node, 
            "Node currently being processed"
        );
        step.metadata.set(
            "depth", 
            state.current_depth, 
            "Current DFS depth"
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
            "stack_size", 
            state.stack.size(), 
            "Current stack size"
        );

        if (m_target_node.has_value())
        {
            step.metadata.set(
                "target_node", 
                m_target_node.value(), 
                "Target node being searched for");
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

        if (state.current_node < state.entry_time.size() && 
            state.current_node != static_cast<size_t>(-1))
        {
            if (state.entry_time[state.current_node] >= 0)
            {
                step.metadata.set(
                    "entry_time", 
                    state.entry_time[state.current_node],
                    "Discovery time of current node"
                );
            }
            if (state.exit_time[state.current_node] >= 0)
            {
                step.metadata.set(
                    "exit_time", 
                    state.exit_time[state.current_node],
                    "Finish time of current node"
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
        }

        if (state.target_found)
        {
            step.metadata.set(
                "found_at_depth", 
                state.current_depth,
                "Depth at which target was found"
            );
            step.metadata.set(
                "found_at_node", 
                state.target_node_found,
                "Target node found"
            );
        }

        // Phase as string
        std::string phase_str;
        switch (state.phase)
        {
            case DFSState::Phase::INITIALIZE: 
                phase_str = "Initializing"; break;
            case DFSState::Phase::PUSH_START: 
                phase_str = "Pushing Start Node"; break;
            case DFSState::Phase::POP_NODE: 
                phase_str = "Popping Node"; break;
            case DFSState::Phase::VISIT_NODE: 
                phase_str = "Visiting Node"; break;
            case DFSState::Phase::EXPLORE_EDGE: 
                phase_str = "Exploring Edge"; break;
            case DFSState::Phase::PUSH_NEIGHBOR: 
                phase_str = "Pushing Neighbor"; break;
            case DFSState::Phase::NODE_PROCESSED: 
                phase_str = "Node Processed"; break;
            case DFSState::Phase::BACKTRACK: 
                phase_str = "Backtracking"; break;
            case DFSState::Phase::STACK_EMPTY: 
                phase_str = "Stack Empty"; break;
            case DFSState::Phase::TARGET_FOUND: 
                phase_str = "Target Found"; break;
            case DFSState::Phase::COMPLETED: 
                phase_str = "Completed"; break;
        }
        step.metadata.set(
            "phase", 
            phase_str, 
            "Current DFS phase"
        );
    }

    void DFS::update_visualization_data(
        AlgorithmStep& step, 
        const DFSState& state, 
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

        // Set frontier nodes (in stack but not visited) - these are future nodes to explore
        // For DFS visualization, the stack is a LIFO structure
        std::stack<size_t> temp_stack = state.stack;
        std::vector<size_t> stack_nodes;
        while (!temp_stack.empty())
        {
            stack_nodes.push_back(temp_stack.top());
            temp_stack.pop();
        }
        // Reverse to show in correct order
        std::reverse(stack_nodes.begin(), stack_nodes.end());
        for (size_t node : stack_nodes)
        {
            if (!state.visited[node])
            {
                viz.graph_state.frontier_nodes.push_back(node);
            }
        }

        // Set active edge
        if (state.phase == DFSState::Phase::EXPLORE_EDGE &&
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
        else if (operation_id == "push_neighbor" && 
            state.current_neighbor != static_cast<size_t>(-1))
        {
            viz.additional_highlights.push_back(state.current_neighbor);
        }
        else if (operation_id == "pop_node" && 
            state.current_node != static_cast<size_t>(-1))
        {
            viz.highlighted_index = state.current_node;
        }
        else if (operation_id == "backtrack" && 
            state.current_node != static_cast<size_t>(-1))
        {
            viz.highlighted_index = state.current_node;
            // Add a subtle highlight to indicate backtracking
            viz.additional_highlights.push_back(state.current_node);
        }

        // Set metrics
        viz.comparison_count = state.comparisons;
        viz.swap_count = 0;
        viz.is_complete = state.is_complete || state.target_found;
    }

    void DFS::reset_state()
    {
        m_current_state = DFSState{};
        m_current_state.visited.resize(
            m_graph.node_count, 
            false
        );
        m_current_state.in_stack.resize(
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
        m_current_state.entry_time.resize(
            m_graph.node_count, 
            -1
        );
        m_current_state.exit_time.resize(
            m_graph.node_count, 
            -1
        );
    }

} // namespace c2l::algorithms