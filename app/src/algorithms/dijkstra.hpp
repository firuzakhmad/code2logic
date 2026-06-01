#ifndef CODE2LOGIC_DIJKSTRA_HPP
#define CODE2LOGIC_DIJKSTRA_HPP

#include "algorithms/core/json_algorithm_base.hpp"
#include "algorithms/core/algorithm_step.hpp"
#include <queue>
#include <vector>
#include <optional>
#include <limits>

namespace c2l::algorithms
{
    /**
     * @brief Dijkstra's shortest path algorithm with JSON-driven metadata
     *
     * Dijkstra's algorithm finds the shortest paths from a source node
     * to all other nodes in a weighted graph with non-negative edge weights.
     */
    class Dijkstra final : public JsonAlgorithmBase
    {
    public:
        explicit Dijkstra(core::JsonConfigManager& json_config_manager);
        ~Dijkstra() override = default;

        Dijkstra(const Dijkstra&) = delete;
        Dijkstra& operator=(const Dijkstra&) = delete;
        Dijkstra(Dijkstra&&) noexcept = delete;
        Dijkstra& operator=(Dijkstra&&) noexcept = delete;

        void generate_all_steps() override;
        void reset_state() override;

        void set_graph_structure(
            const std::vector<std::vector<size_t>>& adjacency_list
        );
        void set_graph_structure_weighted(
            const std::vector<std::vector<std::pair<size_t, int>>>& weighted_adjacency_list
        );
        void set_start_node(
            size_t start
        );
        void set_target_node(
            std::optional<size_t> target = std::nullopt
        );

        // Get current Dijkstra state for visualization
        [[nodiscard]] size_t get_current_node() const 
        { 
            return m_current_state.current_node; 
        }
        [[nodiscard]] size_t get_pq_size() const 
        { 
            return m_current_state.pq.size(); 
        }
        [[nodiscard]] size_t get_explored_count() const 
        { 
            return m_current_state.explored_count; 
        }
        [[nodiscard]] const std::vector<bool>& get_settled() const 
        { 
            return m_current_state.settled; 
        }
        [[nodiscard]] const std::vector<int>& get_distances() const 
        { 
            return m_current_state.distance; 
        }
        [[nodiscard]] const std::vector<size_t>& get_previous() const 
        { 
            return m_current_state.previous; 
        }

    private:
        struct Edge
        {
            size_t to;
            int weight;
            Edge(size_t to, int weight) 
                : to(to), weight(weight) 
            {}
        };

        struct Graph
        {
            std::vector<std::vector<Edge>> adjacency_list;
            size_t node_count{0};
            size_t edge_count{0};
            bool is_directed{false};
            bool is_weighted{true};
            std::vector<int> node_weights;
        };

        struct DijkstraState
        {
            using PQElement = std::pair<int, size_t>;  // (distance, node)
            std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> pq;
            std::vector<bool> settled;
            std::vector<bool> in_pq;
            std::vector<size_t> previous;
            std::vector<int> distance;
            std::vector<size_t> traversal_order;
            size_t current_node{static_cast<size_t>(-1)};
            size_t explored_count{0};
            size_t comparisons{0};
            bool is_complete{false};
            bool target_found{false};
            size_t target_node_found{static_cast<size_t>(-1)};

            // For edge-by-edge visualization
            size_t current_neighbor_index{0};
            size_t current_neighbor{static_cast<size_t>(-1)};
            int current_edge_weight{0};
            int new_distance{0};
            int old_distance{0};
            bool update_occurred{false};

            enum class Phase
            {
                INITIALIZE,
                PUSH_SOURCE,
                EXTRACT_MIN,
                SETTLE_NODE,
                RELAX_EDGE,
                UPDATE_DISTANCE,
                PUSH_NEIGHBOR,
                NODE_SETTLED,
                PQ_EMPTY,
                TARGET_FOUND,
                COMPLETED
            } phase{Phase::INITIALIZE};
        };

        void push_step(
            const DijkstraState& state, 
            const std::string& operation_id
        );
        AlgorithmStep create_step_from_state(
            const DijkstraState& state, 
            const std::string& operation_id
        ) const;
        void populate_step_metadata(
            AlgorithmStep& step, 
            const DijkstraState& state, 
            const std::string& operation_id
        ) const;
        void update_visualization_data(
            AlgorithmStep& step, 
            const DijkstraState& state, 
            const std::string& operation_id
        ) const;

        bool perform_dijkstra_step(
            DijkstraState& state
        );
        void relax_edge(
            DijkstraState& state, 
            size_t neighbor, 
            int weight
        );

        Graph m_graph;
        std::optional<size_t> m_start_node{0};
        std::optional<size_t> m_target_node;
        DijkstraState m_current_state;

        size_t m_total_explored{0};
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_DIJKSTRA_HPP