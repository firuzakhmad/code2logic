#include "algorithms/a_star.hpp"
#include "core/utils/logger/logger.hpp"

#include <algorithm>
#include <sstream>
#include <cmath>

#include "core/utils/variables.hpp"

namespace c2l::algorithms
{
    AStar::AStar(core::JsonConfigManager& json_config_manager)
        : JsonAlgorithmBase(json_config_manager, AlgorithmType::A_STAR)
    {
        LOG_DEBUG("A* created and metadata loaded from JSON");
    }

    void AStar::set_graph_structure(
        const std::vector<std::vector<size_t>>& adjacency_list)
    {
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

    void AStar::set_graph_structure_weighted(
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

    void AStar::set_node_positions(
        const std::vector<std::pair<float, float>>& positions)
    {
        m_graph.node_positions = positions;
        if (m_graph.node_positions.size() < m_graph.node_count)
        {
            m_graph.node_positions.resize(m_graph.node_count, {0.0f, 0.0f});
        }
    }

    void AStar::set_start_node(size_t start)
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

    void AStar::set_target_node(size_t target)
    {
        if (target < m_graph.node_count)
        {
            m_target_node.value() = target;
        }
        else
        {
            LOG_WARNING(
                "Invalid target node: {}, graph has {} nodes", 
                target, 
                m_graph.node_count
            );
        }
        reset();
        generate_all_steps();
    }

    void AStar::set_heuristic_type(HeuristicType type)
    {
        if (type == HeuristicType::Euclidean || 
            type == HeuristicType::Manhattan || 
            type == HeuristicType::Chebyshev)
        {
            m_heuristic_type = type;
        }
        else
        {
            LOG_WARNING(
                "Unknown heuristic type: {}, using euclidean", 
                static_cast<int>(type)
            );
            m_heuristic_type = HeuristicType::Euclidean;
        }
    }

    float AStar::calculate_heuristic(size_t node, size_t target) const
    {
        if (node >= m_graph.node_positions.size() || 
            target >= m_graph.node_positions.size())
        {
            return 0.0f;  // No position data available
        }

        const auto& pos1 = m_graph.node_positions[node];
        const auto& pos2 = m_graph.node_positions[target];
        float dx = pos1.first - pos2.first;
        float dy = pos1.second - pos2.second;

        if (m_heuristic_type == HeuristicType::Euclidean)
        {
            return std::sqrt(dx * dx + dy * dy);
        }
        else if (m_heuristic_type == HeuristicType::Manhattan)
        {
            return std::abs(dx) + std::abs(dy);
        }
        else if (m_heuristic_type == HeuristicType::Chebyshev)
        {
            return std::max(std::abs(dx), std::abs(dy));
        }

        return std::sqrt(dx * dx + dy * dy);
    }

    void AStar::generate_all_steps()
    {
        if (m_graph.node_count == 0)
        {
            LOG_WARNING("Cannot generate A* steps: graph has no nodes");
            return;
        }

        if (m_start_node.value() >= m_graph.node_count)
        {
            LOG_ERROR("Invalid start node: {}", m_start_node.value());
            return;
        }

        if (m_target_node.value() >= m_graph.node_count)
        {
            LOG_ERROR("Invalid target node: {}", m_target_node.value());
            return;
        }

        m_steps.clear();

        // Initialize state
        AStarState state;
        state.in_open_set.resize(m_graph.node_count, false);
        state.closed_set.resize(m_graph.node_count, false);
        state.came_from.resize(m_graph.node_count, static_cast<size_t>(-1));
        state.g_score.resize(m_graph.node_count, core::INF);
        state.f_score.resize(m_graph.node_count, core::INF);
        state.phase = AStarState::Phase::INITIALIZE;
        state.explored_count = 0;
        state.comparisons = 0;
        state.is_complete = false;
        state.target_found = false;
        state.current_node = static_cast<size_t>(-1);
        state.current_neighbor = static_cast<size_t>(-1);
        state.current_edge_weight = 0;
        state.tentative_g_score = 0.0f;
        state.old_g_score = 0.0f;
        state.new_f_score = 0.0f;

        // Initialization
        push_step(state, "init");

        // Pushing source node to open set
        state.g_score[m_start_node.value()] = 0.0f;
        state.f_score[m_start_node.value()] = calculate_heuristic(
            m_start_node.value(), 
            m_target_node.value()
        );
        state.open_set.emplace(
            state.f_score[m_start_node.value()], 
            m_start_node.value()
        );
        state.in_open_set[m_start_node.value()] = true;
        state.visited_count = 1;
        state.phase = AStarState::Phase::PUSH_SOURCE;
        push_step(state, "push_source");

        // Processing A* until complete or target found
        while (!state.is_complete && 
               !state.target_found && 
               !state.open_set.empty())
        {
            // Popping node with minimum f-score
            auto [current_f, current_node] = state.open_set.top();
            state.open_set.pop();
            state.in_open_set[current_node] = false;
            state.current_node = current_node;
            state.phase = AStarState::Phase::POP_MIN;
            push_step(state, "pop_min");

            // Goal check
            state.phase = AStarState::Phase::GOAL_CHECK;
            push_step(state, "goal_check");

            if (current_node == m_target_node.value())
            {
                state.target_found = true;
                state.target_node_found = current_node;
                state.phase = AStarState::Phase::TARGET_FOUND;
                push_step(state, "target_found");
                break;
            }

            // Adding to closed set
            state.closed_set[current_node] = true;
            state.explored_count++;
            state.traversal_order.push_back(current_node);
            state.phase = AStarState::Phase::ADD_TO_CLOSED;
            push_step(state, "add_to_closed");

            // Evaluating all edges from current node
            const auto& edges = m_graph.adjacency_list[current_node];

            for (size_t i = 0; i < edges.size(); ++i)
            {
                const Edge& edge = edges[i];
                size_t neighbor = edge.to;
                int weight = edge.weight;

                // Skipping if in closed set
                if (state.closed_set[neighbor])
                {
                    continue;
                }

                state.current_neighbor_index = i;
                state.current_neighbor = neighbor;
                state.current_edge_weight = weight;
                state.comparisons++;
                state.phase = AStarState::Phase::EVALUATE_EDGE;
                push_step(state, "evaluate_edge");

                float tentative_g = state.g_score[current_node] + weight;
                state.tentative_g_score = tentative_g;
                state.old_g_score = state.g_score[neighbor];

                // Relax edge if better path found
                state.phase = AStarState::Phase::RELAX_EDGE;
                push_step(state, "relax_edge");

                if (tentative_g < state.g_score[neighbor])
                {
                    state.came_from[neighbor] = current_node;
                    state.g_score[neighbor] = tentative_g;
                    state.new_f_score = 
                        state.g_score[neighbor] + 
                            calculate_heuristic(neighbor, m_target_node.value());
                    state.f_score[neighbor] = state.new_f_score;

                    state.phase = AStarState::Phase::UPDATE_SCORES;
                    push_step(state, "update_scores");

                    if (!state.in_open_set[neighbor])
                    {
                        state.open_set.emplace(state.f_score[neighbor], neighbor);
                        state.in_open_set[neighbor] = true;
                        state.visited_count++;

                        state.phase = AStarState::Phase::PUSH_NEIGHBOR;
                        push_step(state, "push_neighbor");
                    }
                }
            }

            state.current_neighbor = static_cast<size_t>(-1);
            state.current_neighbor_index = 0;
        }

        // Final steps
        if (state.open_set.empty() && !state.target_found)
        {
            state.is_complete = true;
            state.phase = AStarState::Phase::OPEN_SET_EMPTY;
            push_step(state, "open_set_empty");
        }

        state.phase = AStarState::Phase::COMPLETED;
        push_step(state, "completed");

        LOG_DEBUG("Generated {} steps for A*", m_steps.size());
    }

    void AStar::push_step(
        const AStarState& state, 
        const std::string& operation_id)
    {
        auto step = create_step_from_state(state, operation_id);
        m_steps.push_back(std::move(step));
    }

    AlgorithmStep AStar::create_step_from_state(
        const AStarState& state, 
        const std::string& operation_id
    ) const 
    {
        AlgorithmStep step;

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

    void AStar::populate_step_metadata(
        AlgorithmStep& step, 
        const AStarState& state, 
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
            "Source node for A*"
        );
        step.metadata.set(
            "target_node", 
            m_target_node.value(), 
            "Target node for A*"
        );
        step.metadata.set(
            "current_node", 
            state.current_node, 
            "Node currently being processed"
        );
        step.metadata.set(
            "closed_set_size", 
            state.explored_count, 
            "Number of nodes in closed set"
        );
        step.metadata.set(
            "open_set_size", 
            state.open_set.size(), 
            "Current open set size"
        );
        step.metadata.set(
            "comparisons", 
            state.comparisons, 
            "Number of edge evaluations"
        );

        if (operation_id == "push_source")
        {
            step.metadata.set(
                "f_score",
                state.f_score[m_start_node.value()],
                "Source node f-score"
            );
        }

        if (state.current_node < state.g_score.size() && 
            state.current_node != static_cast<size_t>(-1))
        {
            if (state.g_score[state.current_node] < core::INF)
            {
                step.metadata.set(
                    "g_score", 
                    state.g_score[state.current_node],
                    "Actual cost from source to current node"
                );
            }
            if (state.f_score[state.current_node] < core::INF)
            {
                step.metadata.set(
                    "f_score", 
                    state.f_score[state.current_node],
                    "Estimated total cost"
                );
            }
        }

        if (state.current_neighbor != static_cast<size_t>(-1))
        {
            step.metadata.set(
                "explored_neighbor", 
                state.current_neighbor,
                "Currently evaluating neighbor"
            );
            step.metadata.set(
                "edge_weight", 
                state.current_edge_weight,
                "Weight of edge being evaluated"
            );
            step.metadata.set(

                "new_g_score",
                state.g_score[state.current_neighbor],
                "Updated g-score for neighbor"
            );

            if (state.f_score[state.current_neighbor] < core::INF)
            {
                step.metadata.set(

                    "f_score",
                    state.f_score[state.current_neighbor],
                    "F-score of current neighbor"
                );
            }

            if (state.tentative_g_score < core::INF)
            {
                step.metadata.set(
                    "tentative_g_score", state.tentative_g_score,
                    "Potential new g-score"
                );
            }
            if (state.old_g_score < core::INF)
            {
                step.metadata.set(
                    "old_g_score", state.old_g_score,
                    "Current g-score of neighbor"
                );
            }
            else
            {
                step.metadata.set(
                    "old_g_score", "INFINIT",
                    "Current g-score of neighbor"
                );
            }

            if (state.new_f_score < core::INF)
            {
                step.metadata.set(
                    "new_f_score", state.new_f_score,
                    "New f-score for neighbor"
                );
            }
            else 
            {
                step.metadata.set(
                    "new_f_score", "INFINIT",
                    "New f-score for neighbor"
                );
            }
        }

        if (state.target_found)
        {
            step.metadata.set(
                "found_at_cost", 
                state.g_score[m_target_node.value()],
                "Total path cost to target"
            );
            step.metadata.set(
                "found_at_node", 
                m_target_node.value(),
                "Target node found"
            );
        }

        // Phase as string
        std::string phase_str;
        switch (state.phase)
        {
            case AStarState::Phase::INITIALIZE: 
                phase_str = "Initializing"; break;
            case AStarState::Phase::PUSH_SOURCE: 
                phase_str = "Pushing Source Node"; break;
            case AStarState::Phase::POP_MIN: 
                phase_str = "Popping Minimum F-Score"; break;
            case AStarState::Phase::GOAL_CHECK: 
                phase_str = "Goal Check"; break;
            case AStarState::Phase::ADD_TO_CLOSED: 
                phase_str = "Adding to Closed Set"; break;
            case AStarState::Phase::EVALUATE_EDGE: 
                phase_str = "Evaluating Edge"; break;
            case AStarState::Phase::RELAX_EDGE: 
                phase_str = "Relaxing Edge"; break;
            case AStarState::Phase::UPDATE_SCORES: 
                phase_str = "Updating Scores"; break;
            case AStarState::Phase::PUSH_NEIGHBOR: 
                phase_str = "Pushing to Open Set"; break;
            case AStarState::Phase::TARGET_FOUND: 
                phase_str = "Target Found"; break;
            case AStarState::Phase::OPEN_SET_EMPTY: 
                phase_str = "Open Set Empty"; break;
            case AStarState::Phase::COMPLETED: 
                phase_str = "Completed"; break;
        }
        step.metadata.set(
            "phase", 
            phase_str, 
            "Current A* phase"
        );
    }

    void AStar::update_visualization_data(
        AlgorithmStep& step, 
        const AStarState& state, 
        const std::string& operation_id
    ) const
    {
        auto& viz = step.visualization;

        viz.graph_state.visited_nodes.clear();
        viz.graph_state.frontier_nodes.clear();
        viz.graph_state.active_edges.clear();
        viz.graph_state.path.clear();
        viz.graph_state.node_distances.clear();
        viz.graph_state.node_parents.clear();
        viz.highlighted_index.reset();
        viz.compared_index.reset();
        viz.additional_highlights.clear();

        // Closed set nodes (explored/visited)
        for (size_t i = 0; i < state.closed_set.size(); ++i)
        {
            if (state.closed_set[i])
            {
                viz.graph_state.visited_nodes.push_back(i);
                if (state.g_score[i] < core::INF)
                {
                    viz.graph_state.node_distances[i] = static_cast<int>(state.g_score[i]);
                }
                if (state.came_from[i] != static_cast<size_t>(-1))
                {
                    viz.graph_state.node_parents[i] = state.came_from[i];
                }
            }
        }

        // Open set nodes (frontier)
        for (size_t i = 0; i < state.in_open_set.size(); ++i)
        {
            if (state.in_open_set[i] && !state.closed_set[i])
            {
                viz.graph_state.frontier_nodes.push_back(i);
            }
        }

        // Active edge
        if ((state.phase == AStarState::Phase::EVALUATE_EDGE ||
             state.phase == AStarState::Phase::RELAX_EDGE) &&
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
            m_target_node.value() < state.came_from.size())
        {
            std::vector<size_t> path;
            size_t current = m_target_node.value();
            while (current != static_cast<size_t>(-1))
            {
                path.push_back(current);
                current = state.came_from[current];
            }
            std::reverse(path.begin(), path.end());
            viz.graph_state.path = path;
        }

        // Step-specific highlights
        if ((operation_id == "add_to_closed" || operation_id == "pop_min") && 
            state.current_node != static_cast<size_t>(-1))
        {
            viz.highlighted_index = state.current_node;
        }
        else if (operation_id == "evaluate_edge" && 
                 state.current_neighbor != static_cast<size_t>(-1))
        {
            viz.compared_index = state.current_node;
            viz.additional_highlights.push_back(state.current_neighbor);
        }
        else if (operation_id == "update_scores" && 
                 state.current_neighbor != static_cast<size_t>(-1))
        {
            viz.additional_highlights.push_back(state.current_neighbor);
        }
        else if (operation_id == "push_neighbor" && 
                 state.current_neighbor != static_cast<size_t>(-1))
        {
            viz.additional_highlights.push_back(state.current_neighbor);
        }

        viz.comparison_count = state.comparisons;
        viz.explored_node_count = state.explored_count;
        viz.visited_node_count = state.visited_count;
        viz.swap_count = 0;
        viz.is_complete = state.is_complete || state.target_found;
    }

    void AStar::reset_state()
    {
        m_current_state = AStarState{};
        m_current_state.in_open_set.resize(
            m_graph.node_count, 
            false
        );
        m_current_state.closed_set.resize(
            m_graph.node_count, 
            false
        );
        m_current_state.came_from.resize(
            m_graph.node_count, 
            static_cast<size_t>(-1)
        );
        m_current_state.g_score.resize(
            m_graph.node_count, 
            core::INF
        );
        m_current_state.f_score.resize(
            m_graph.node_count, 
            core::INF
        );
    }

} // namespace c2l::algorithms