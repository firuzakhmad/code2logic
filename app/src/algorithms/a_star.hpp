#ifndef CODE2LOGIC_ASTAR_HPP
#define CODE2LOGIC_ASTAR_HPP

#include "algorithms/core/json_algorithm_base.hpp"
#include "algorithms/core/algorithm_step.hpp"
#include <queue>
#include <vector>
#include <optional>
#include <cmath>
#include <functional>
#include <limits>

namespace c2l::algorithms
{
    /**
     * @brief A* Search algorithm with JSON-driven metadata
     *
     * A* is an informed graph search algorithm that uses a heuristic
     * to efficiently find the shortest path from source to target.
     */
    class AStar final : public JsonAlgorithmBase
    {
    public:
        explicit AStar(core::JsonConfigManager& json_config_manager);
        ~AStar() override = default;

        AStar(const AStar&) = delete;
        AStar& operator=(const AStar&) = delete;
        AStar(AStar&&) noexcept = delete;
        AStar& operator=(AStar&&) noexcept = delete;

        void reset_state() override;
        void generate_all_steps() override;

        void set_graph_structure(
          const std::vector<std::vector<size_t>>& adjacency_list
        );
        void set_graph_structure_weighted(
          const std::vector<std::vector<std::pair<size_t, int>>>& weighted_adjacency_list
        );
        void set_node_positions(
          const std::vector<std::pair<float, float>>& positions
        );
        void set_start_node(size_t start);
        void set_target_node(size_t target);
        void set_heuristic_type(HeuristicType type) override;

        // Get current A* state for visualization
        [[nodiscard]] size_t get_current_node() const
        { 
          return m_current_state.current_node; 
        }

        [[nodiscard]] size_t get_open_set_size() const override
        { 
          return m_current_state.open_set.size(); 
        }

        [[nodiscard]] size_t get_closed_set_size() const override
        { 
          return m_current_state.explored_count; 
        }

        [[nodiscard]] const std::vector<bool>& get_closed_set() const 
        { 
          return m_current_state.closed_set; 
        }

        [[nodiscard]] const std::vector<float>& get_g_scores() const 
        { 
          return m_current_state.g_score; 
        }

        [[nodiscard]] const std::vector<float>& get_f_scores() const 
        { 
          return m_current_state.f_score; 
        }

    private:
        struct Edge
        {
            size_t to;
            int weight;
            Edge(size_t to, int weight) : to(to), weight(weight) {}
        };

        struct Graph
        {
            std::vector<std::vector<Edge>> adjacency_list;
            std::vector<std::pair<float, float>> node_positions;
            size_t node_count{0};
            size_t edge_count{0};
            bool is_directed{false};
            bool is_weighted{true};
            std::vector<int> node_weights;
        };

        struct AStarState
        {
            using PQElement = std::pair<float, size_t>;  // (f_score, node)
            std::priority_queue<
              PQElement,
              std::vector<PQElement>,
              std::greater<PQElement>
            > open_set;
            std::vector<bool> in_open_set;
            std::vector<bool> closed_set;
            std::vector<size_t> came_from;
            std::vector<float> g_score;
            std::vector<float> f_score;
            std::vector<size_t> traversal_order;
            size_t current_node{static_cast<size_t>(-1)};
            size_t explored_count{0};  // closed set size
            size_t visited_count{0};
            size_t comparisons{0};
            bool is_complete{false};
            bool target_found{false};
            size_t target_node_found{static_cast<size_t>(-1)};

            // For edge-by-edge visualization
            size_t current_neighbor_index{0};
            size_t current_neighbor{static_cast<size_t>(-1)};
            int current_edge_weight{0};
            float tentative_g_score{0.0f};
            float old_g_score{0.0f};
            float new_f_score{0.0f};

            enum class Phase
            {
                INITIALIZE,
                PUSH_SOURCE,
                POP_MIN,
                GOAL_CHECK,
                ADD_TO_CLOSED,
                EVALUATE_EDGE,
                RELAX_EDGE,
                UPDATE_SCORES,
                PUSH_NEIGHBOR,
                TARGET_FOUND,
                OPEN_SET_EMPTY,
                COMPLETED
            } phase{Phase::INITIALIZE};
        };

        float calculate_heuristic(
          size_t node, 
          size_t target
        ) const;
        void push_step(
          const AStarState& state, 
          const std::string& 
          operation_id
        );
        AlgorithmStep create_step_from_state(
          const AStarState& state, 
          const std::string& operation_id
        ) const;
        void populate_step_metadata(
          AlgorithmStep& step, 
          const AStarState& state, 
          const std::string& operation_id
        ) const;
        void update_visualization_data(
          AlgorithmStep& step, 
          const AStarState& state, 
          const std::string& operation_id
        ) const;

        Graph m_graph;
        HeuristicType m_heuristic_type{HeuristicType::Euclidean};
        AStarState m_current_state;
        std::optional<size_t> m_start_node{0};
        std::optional<size_t> m_target_node{0};
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_ASTAR_HPP