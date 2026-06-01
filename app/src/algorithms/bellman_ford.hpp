#ifndef CODE2LOGIC_BELLMAN_FORD_HPP
#define CODE2LOGIC_BELLMAN_FORD_HPP

#include "algorithms/core/json_algorithm_base.hpp"
#include "algorithms/core/algorithm_step.hpp"
#include <vector>
#include <optional>
#include <limits>

namespace c2l::algorithms
{
    /**
     * @brief Bellman-Ford shortest path algorithm with JSON-driven metadata
     *
     * Bellman-Ford finds shortest paths from a source vertex to all other vertices
     * in a weighted graph. Unlike Dijkstra, it handles negative edge weights and
     * can detect negative cycles.
     */
    class BellmanFord final : public JsonAlgorithmBase
    {
    public:
        explicit BellmanFord(
            core::JsonConfigManager& json_config_manager
        );
        ~BellmanFord() override = default;

        BellmanFord(const BellmanFord&) = delete;
        BellmanFord& operator=(const BellmanFord&) = delete;
        BellmanFord(BellmanFord&&) noexcept = delete;
        BellmanFord& operator=(BellmanFord&&) noexcept = delete;

        void reset_state() override;
        void generate_all_steps() override;

        void set_graph_structure(
            const std::vector<std::vector<size_t>>& adjacency_list
        );
        void set_graph_structure_weighted(
            const std::vector<std::vector<std::pair<size_t, int>>>& weighted_adjacency_list
        );
        void set_start_node(size_t start);
        void set_target_node(
            std::optional<size_t> target = std::nullopt
        );

        // Get current state for visualization
        [[nodiscard]] size_t get_current_node() const 
        { 
            return m_current_state.current_node; 
        }
        [[nodiscard]] size_t get_current_neighbor() const 
        { 
            return m_current_state.current_neighbor; 
        }
        [[nodiscard]] size_t get_current_iteration() const 
        { 
            return m_current_state.current_iteration; 
        }
        [[nodiscard]] size_t get_explored_count() const 
        { 
            return m_current_state.explored_count; 
        }
        [[nodiscard]] const std::vector<int>& get_distances() const 
        { 
            return m_current_state.distance; 
        }
        [[nodiscard]] const std::vector<size_t>& get_previous() const 
        { 
            return m_current_state.previous; 
        }
        [[nodiscard]] bool has_negative_cycle() const 
        { 
            return m_current_state.negative_cycle_detected; 
        }

    private:
        struct Edge
        {
            size_t from;
            size_t to;
            int weight;
            Edge(size_t f, size_t t, int w) 
                : from(f), to(t), weight(w) 
            {}
        };

        struct Graph
        {
            std::vector<Edge> edges;
            std::vector<std::vector<std::pair<size_t, int>>> adjacency_list;
            size_t node_count{0};
            size_t edge_count{0};
            bool is_directed{true};
            bool is_weighted{true};
            std::vector<int> node_weights;
        };

        struct BellmanFordState
        {
            std::vector<int> distance;
            std::vector<size_t> previous;
            std::vector<bool> reached;
            std::vector<size_t> traversal_order;
            
            size_t current_iteration{0};
            size_t current_edge_index{0};
            size_t current_node{static_cast<size_t>(-1)};
            size_t current_neighbor{static_cast<size_t>(-1)};
            int current_edge_weight{0};
            int old_distance{0};
            int new_distance{0};
            
            size_t explored_count{0};
            size_t visited_count{0};
            size_t comparisons{0};
            size_t updates_in_iteration{0};
            bool updates_occurred{false};
            bool negative_cycle_detected{false};
            bool negative_cycle_checked{false};
            bool is_complete{false};
            bool target_found{false};
            size_t target_node_found{static_cast<size_t>(-1)};

            enum class Phase
            {
                INITIALIZE,
                ITERATION_START,
                RELAX_EDGE,
                UPDATE_DISTANCE,
                ITERATION_END,
                NO_UPDATES,
                NEGATIVE_CYCLE_CHECK,
                NEGATIVE_CYCLE_DETECTED,
                TARGET_FOUND,
                COMPLETED
            } phase{Phase::INITIALIZE};
        };
        void push_step(
            const BellmanFordState& state, 
            const std::string& operation_id
        );
        AlgorithmStep create_step_from_state(
            const BellmanFordState& state, 
            const std::string& operation_id
        ) const;
        void populate_step_metadata(
            AlgorithmStep& step, 
            const BellmanFordState& state, 
            const std::string& operation_id
        ) const;
        void update_visualization_data(
            AlgorithmStep& step, 
            const BellmanFordState& state, 
            const std::string& operation_id
        ) const;

        Graph m_graph;
        std::optional<size_t> m_start_node{0};
        std::optional<size_t> m_target_node;
        BellmanFordState m_current_state;
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_BELLMAN_FORD_HPP