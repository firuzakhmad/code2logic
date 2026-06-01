#ifndef CODE2LOGIC_TOPOLOGICAL_SORT_HPP
#define CODE2LOGIC_TOPOLOGICAL_SORT_HPP

#include "algorithms/core/json_algorithm_base.hpp"
#include "algorithms/core/algorithm_step.hpp"
#include <queue>
#include <vector>
#include <optional>

namespace c2l::algorithms
{
    /**
     * @brief Topological Sort algorithm with JSON-driven metadata
     *
     * Topological Sort orders vertices in a Directed Acyclic Graph (DAG)
     * such that for every directed edge u→v, u comes before v.
     * Uses Kahn's algorithm with indegree tracking.
     */
    class TopologicalSort final : public JsonAlgorithmBase
    {
    public:
        explicit TopologicalSort(core::JsonConfigManager& json_config_manager);
        ~TopologicalSort() override = default;

        TopologicalSort(const TopologicalSort&) = delete;
        TopologicalSort& operator=(const TopologicalSort&) = delete;
        TopologicalSort(TopologicalSort&&) noexcept = delete;
        TopologicalSort& operator=(TopologicalSort&&) noexcept = delete;


        void generate_all_steps() override;
        void reset_state() override;

        void set_graph_structure(
            const std::vector<std::vector<size_t>>& adjacency_list
        );
        void set_start_node(size_t start);  // Not used, but required by interface
        void set_target_node(
            std::optional<size_t> target = std::nullopt
        );
        void set_graph_directed(bool directed);

        // Get current state for visualization
        [[nodiscard]] size_t get_current_node() const 
        { 
            return m_current_state.current_node; 
        }
        [[nodiscard]] size_t get_current_neighbor() const 
        { 
            return m_current_state.current_neighbor; 
        }
        [[nodiscard]] size_t get_queue_size() const 
        { 
            return m_current_state.queue.size(); 
        }
        [[nodiscard]] size_t get_sorted_count() const 
        { 
            return m_current_state.sorted_count; 
        }
        [[nodiscard]] const std::vector<size_t>& get_topological_order() const 
        { 
            return m_current_state.result; 
        }
        [[nodiscard]] const std::vector<int>& get_indegrees() const 
        { 
            return m_current_state.indegree; 
        }
        [[nodiscard]] bool has_cycle() const 
        { 
            return m_current_state.cycle_detected; 
        }

    private:
        struct Edge
        {
            size_t from;
            size_t to;
            Edge(size_t f, size_t t) : from(f), to(t) {}
        };

        struct Graph
        {
            std::vector<std::vector<size_t>> adjacency_list;
            std::vector<Edge> edges;
            size_t node_count{0};
            size_t edge_count{0};
            bool is_directed{true};
            std::vector<int> node_weights;
        };

        struct TopologicalState
        {
            std::vector<int> indegree;
            std::vector<bool> processed;
            std::vector<size_t> result;
            std::queue<size_t> queue;
            
            size_t current_node{static_cast<size_t>(-1)};
            size_t current_neighbor{static_cast<size_t>(-1)};
            size_t sorted_count{0};
            size_t comparisons{0};
            bool cycle_detected{false};
            bool is_complete{false};
            bool target_found{false};
            size_t target_node_found{static_cast<size_t>(-1)};

            enum class Phase
            {
                INITIALIZE,
                COMPUTE_INDEGREES,
                ENQUEUE_ZERO_INDEGREE,
                DEQUEUE_NODE,
                ADD_TO_RESULT,
                TARGET_CHECK,
                TARGET_FOUND,
                DECREMENT_INDEGREE,
                ENQUEUE_NEIGHBOR,
                CYCLE_DETECTED,
                COMPLETED
            } phase{Phase::INITIALIZE};
        };

        void push_step(
            const TopologicalState& state, 
            const std::string& operation_id
        );
        AlgorithmStep create_step_from_state(
            const TopologicalState& state, 
            const std::string& operation_id
        ) const;
        void populate_step_metadata(
            AlgorithmStep& step, 
            const TopologicalState& state, 
            const std::string& operation_id
        ) const;
        void update_visualization_data(
            AlgorithmStep& step, 
            const TopologicalState& state, 
            const std::string& operation_id
        ) const;

        Graph m_graph;
        std::optional<size_t> m_target_node;
        TopologicalState m_current_state;
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_TOPOLOGICAL_SORT_HPP