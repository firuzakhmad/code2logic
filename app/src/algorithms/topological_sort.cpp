#include "algorithms/topological_sort.hpp"
#include "core/utils/logger/logger.hpp"

#include <algorithm>
#include <sstream>
#include <queue>

namespace c2l::algorithms
{
    TopologicalSort::TopologicalSort(core::JsonConfigManager& json_config_manager)
        : JsonAlgorithmBase(json_config_manager, AlgorithmType::TOPOLOGICAL_SORT)
    {
        LOG_DEBUG("TopologicalSort created and metadata loaded from JSON");
    }

    void TopologicalSort::set_graph_structure(const std::vector<std::vector<size_t>>& adjacency_list)
    {
        m_graph.adjacency_list = adjacency_list;
        m_graph.edges.clear();
        
        for (size_t i = 0; i < adjacency_list.size(); ++i)
        {
            for (size_t neighbor : adjacency_list[i])
            {
                m_graph.edges.emplace_back(i, neighbor);
            }
        }

        m_graph.node_count = adjacency_list.size();
        m_graph.edge_count = m_graph.edges.size();

        LOG_DEBUG("Graph structure set: {} nodes, {} edges", m_graph.node_count, m_graph.edge_count);
    }

    void TopologicalSort::set_start_node(size_t /*start*/)
    {
        // Start node not used in topological sort, but required by interface
        LOG_DEBUG("TopologicalSort does not use start node");
    }

    void TopologicalSort::set_target_node(std::optional<size_t> target)
    {
        m_target_node = target;
        reset();
        generate_all_steps();
    }

    void TopologicalSort::set_graph_directed(bool directed)
    {
        m_graph.is_directed = directed;
        if (!directed)
        {
            LOG_WARNING("Topological sort typically requires a directed graph");
        }
    }

    void TopologicalSort::generate_all_steps()
    {
        if (m_graph.node_count == 0)
        {
            LOG_WARNING("Cannot generate TopologicalSort steps: graph has no nodes");
            return;
        }

        m_steps.clear();

        // Initialize state
        TopologicalState state;
        state.indegree.resize(m_graph.node_count, 0);
        state.processed.resize(m_graph.node_count, false);
        state.phase = TopologicalState::Phase::INITIALIZE;
        state.sorted_count = 0;
        state.comparisons = 0;
        state.is_complete = false;
        state.target_found = false;
        state.cycle_detected = false;
        state.current_node = static_cast<size_t>(-1);
        state.current_neighbor = static_cast<size_t>(-1);

        // Step 1: Initialization
        push_step(state, "init");

        // Step 2: Compute indegrees
        state.phase = TopologicalState::Phase::COMPUTE_INDEGREES;
        for (const auto& edge : m_graph.edges)
        {
            state.indegree[edge.to]++;
            state.comparisons++;
        }
        push_step(state, "compute_indegrees");

        // Step 3: Enqueue nodes with indegree 0
        state.phase = TopologicalState::Phase::ENQUEUE_ZERO_INDEGREE;
        for (size_t i = 0; i < m_graph.node_count; ++i)
        {
            if (state.indegree[i] == 0)
            {
                state.current_node = i;
                state.queue.push(i);
                state.processed[i] = true;
                push_step(state, "enqueue_zero_indegree");
            }
        }
        state.current_node = static_cast<size_t>(-1);

        // Process queue
        while (!state.queue.empty() && !state.target_found && !state.cycle_detected)
        {
            // Dequeue node
            state.current_node = state.queue.front();
            state.queue.pop();
            state.phase = TopologicalState::Phase::DEQUEUE_NODE;
            push_step(state, "dequeue_node");

            // Add to result
            state.result.push_back(state.current_node);
            state.sorted_count++;
            state.phase = TopologicalState::Phase::ADD_TO_RESULT;
            push_step(state, "add_to_result");

            // Target check
            state.phase = TopologicalState::Phase::TARGET_CHECK;
            push_step(state, "target_check");

            if (m_target_node.has_value() && state.current_node == m_target_node.value())
            {
                state.target_found = true;
                state.target_node_found = state.current_node;
                state.phase = TopologicalState::Phase::TARGET_FOUND;
                push_step(state, "target_found");
                break;
            }

            // Decrease indegree of neighbors
            for (size_t neighbor : m_graph.adjacency_list[state.current_node])
            {
                state.current_neighbor = neighbor;
                state.indegree[neighbor]--;
                state.comparisons++;

                state.phase = TopologicalState::Phase::DECREMENT_INDEGREE;
                push_step(state, "decrement_indegree");

                if (state.indegree[neighbor] == 0 && !state.processed[neighbor])
                {
                    state.queue.push(neighbor);
                    state.processed[neighbor] = true;
                    state.phase = TopologicalState::Phase::ENQUEUE_NEIGHBOR;
                    push_step(state, "enqueue_neighbor");
                }
            }
            state.current_neighbor = static_cast<size_t>(-1);
        }

        // Check for cycle
        if (!state.target_found && state.result.size() != m_graph.node_count)
        {
            state.cycle_detected = true;
            state.phase = TopologicalState::Phase::CYCLE_DETECTED;
            push_step(state, "cycle_detected");
        }

        state.is_complete = true;
        state.phase = TopologicalState::Phase::COMPLETED;
        push_step(state, "completed");

        LOG_DEBUG("Generated {} steps for TopologicalSort", m_steps.size());
    }

    void TopologicalSort::push_step(const TopologicalState& state, const std::string& operation_id)
    {
        auto step = create_step_from_state(state, operation_id);
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep TopologicalSort::create_step_from_state(const TopologicalState& state, const std::string& operation_id) const
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

    void TopologicalSort::populate_step_metadata(AlgorithmStep& step, const TopologicalState& state, const std::string& operation_id) const
    {
        step.metadata.set("node_count", m_graph.node_count, "Total nodes in graph");
        step.metadata.set("edge_count", m_graph.edge_count, "Total edges in graph");
        step.metadata.set("sorted_count", state.sorted_count, "Number of nodes in topological order");
        step.metadata.set("queue_size", state.queue.size(), "Current queue size");
        step.metadata.set("comparisons", state.comparisons, "Number of edges processed");
        step.metadata.set("result", state.result, "Result");

        if (m_target_node.has_value())
        {
            step.metadata.set("target_node", m_target_node.value(), "Target node for early termination");
        }
        else
        {
            step.metadata.set("target_node", "NULL", "Target node for early termination");
        }

        if (state.current_node != static_cast<size_t>(-1))
        {
            step.metadata.set("current_node", state.current_node, "Node currently being processed");
        }

        if (state.current_neighbor != static_cast<size_t>(-1))
        {
            step.metadata.set("explored_neighbor", state.current_neighbor,
                "Neighbor whose indegree is being decreased");
            if (state.current_neighbor < state.indegree.size())
            {

            }
        }

        if (operation_id == "decrement_indegree")
        {
            step.metadata.set("indegree", state.indegree[state.current_neighbor],
                    "Current indegree of neighbor");
        }
        LOG_DEBUG("Indegree of neighbor {}", state.indegree[state.current_neighbor]);

        if (state.target_found)
        {
            step.metadata.set("found_at_node", state.target_node_found,
                "Target node found in topological order");
        }

        if (state.cycle_detected)
        {
            step.metadata.set("cycle_detected", true,
                "Graph contains a cycle - not a DAG");
        }

        // Build result string for display
        std::string result_str;
        for (size_t i = 0; i < state.result.size(); ++i)
        {
            if (i > 0) result_str += " --> ";
            result_str += std::to_string(state.result[i]);
        }
        if (!result_str.empty())
        {
            step.metadata.set("topological_order", result_str,
                "Current topological order");
        }

        // Phase as string
        std::string phase_str;
        switch (state.phase)
        {
            case TopologicalState::Phase::INITIALIZE: phase_str = "Initializing"; break;
            case TopologicalState::Phase::COMPUTE_INDEGREES: phase_str = "Computing Indegrees"; break;
            case TopologicalState::Phase::ENQUEUE_ZERO_INDEGREE: phase_str = "Enqueuing Zero-Indegree Nodes"; break;
            case TopologicalState::Phase::DEQUEUE_NODE: phase_str = "Dequeuing Node"; break;
            case TopologicalState::Phase::ADD_TO_RESULT: phase_str = "Adding to Result"; break;
            case TopologicalState::Phase::TARGET_CHECK: phase_str = "Checking Target"; break;
            case TopologicalState::Phase::TARGET_FOUND: phase_str = "Target Found!"; break;
            case TopologicalState::Phase::DECREMENT_INDEGREE: phase_str = "Decrementing Indegree"; break;
            case TopologicalState::Phase::ENQUEUE_NEIGHBOR: phase_str = "Enqueuing Neighbor"; break;
            case TopologicalState::Phase::CYCLE_DETECTED: phase_str = "Cycle Detected!"; break;
            case TopologicalState::Phase::COMPLETED: phase_str = "Completed"; break;
        }
        step.metadata.set("phase", phase_str, "Current Topological Sort phase");
    }

    void TopologicalSort::update_visualization_data(AlgorithmStep& step, const TopologicalState& state, const std::string& operation_id) const
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

        // Processed nodes are "visited"
        for (size_t i = 0; i < state.result.size(); ++i)
        {
            viz.graph_state.visited_nodes.push_back(state.result[i]);
        }

        // Queued nodes are "frontier"
        std::queue<size_t> temp_queue = state.queue;
        while (!temp_queue.empty())
        {
            viz.graph_state.frontier_nodes.push_back(temp_queue.front());
            temp_queue.pop();
        }

        // Store indegree values in node_distances for display
        for (size_t i = 0; i < state.indegree.size(); ++i)
        {
            if (state.indegree[i] >= 0)
            {
                viz.graph_state.node_distances[i] = state.indegree[i];
            }
        }

        // Store parent information (which node caused the indegree decrement)
        // For topological sort, we don't have traditional parents, but we can show
        // the edge being processed
        if (state.current_node != static_cast<size_t>(-1) &&
            state.current_neighbor != static_cast<size_t>(-1))
        {
            viz.graph_state.node_parents[state.current_neighbor] = state.current_node;
        }

        // Active edge
        if (operation_id == "decrement_indegree" &&
            state.current_node != static_cast<size_t>(-1) &&
            state.current_neighbor != static_cast<size_t>(-1))
        {
            viz.graph_state.active_edges.emplace_back(state.current_node, state.current_neighbor);
        }

        // Set step-specific highlights
        if (operation_id == "add_to_result" && state.current_node != static_cast<size_t>(-1))
        {
            viz.highlighted_index = state.current_node;
        }
        else if (operation_id == "decrement_indegree" && state.current_neighbor != static_cast<size_t>(-1))
        {
            viz.compared_index = state.current_node;
            viz.additional_highlights.push_back(state.current_neighbor);
        }
        else if (operation_id == "enqueue_neighbor" && state.current_neighbor != static_cast<size_t>(-1))
        {
            viz.additional_highlights.push_back(state.current_neighbor);
        }
        else if (operation_id == "dequeue_node" && state.current_node != static_cast<size_t>(-1))
        {
            viz.highlighted_index = state.current_node;
        }

        // TODO:
        // Set path = topological order (only when target found or complete)
        if (state.target_found || state.is_complete)
        {
            viz.graph_state.path = state.result;
        }

        // Set metrics
        viz.comparison_count = state.comparisons;
        viz.visited_node_count = viz.graph_state.visited_nodes.size();
        viz.swap_count = 0;
        viz.is_complete = state.is_complete || state.target_found || state.cycle_detected;
    }

    void TopologicalSort::reset_state()
    {
        m_current_state = TopologicalState{};
        m_current_state.indegree.resize(m_graph.node_count, 0);
        m_current_state.processed.resize(m_graph.node_count, false);
    }

} // namespace c2l::algorithms