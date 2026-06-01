#ifndef CODE2LOGIC_DFS_HPP
#define CODE2LOGIC_DFS_HPP

#include "algorithms/core/json_algorithm_base.hpp"
#include "algorithms/core/algorithm_step.hpp"
#include <stack>
#include <vector>
#include <optional>

namespace c2l::algorithms
{
    /**
     * @brief Depth-First Search algorithm with JSON-driven metadata
     *
     * DFS traverses a graph depth-first, exploring as far as possible
     * along each branch before backtracking.
     */
    class DFS final : public JsonAlgorithmBase
    {
    public:
        explicit DFS(core::JsonConfigManager& json_config_manager);
        ~DFS() override = default;

        DFS(const DFS&) = delete;
        DFS& operator=(const DFS&) = delete;
        DFS(DFS&&) noexcept = delete;
        DFS& operator=(DFS&&) noexcept = delete;

        void generate_all_steps() override;
        void reset_state() override;

        void set_graph_structure(
            const std::vector<std::vector<size_t>>& adjacency_list
        );
        void set_start_node(size_t start);
        void set_target_node(
            std::optional<size_t> target = std::nullopt
        );

        // Get current DFS state for visualization
        [[nodiscard]] size_t get_current_node() const 
        { 
            return m_current_state.current_node; 
        }
        [[nodiscard]] size_t get_stack_size() const 
        { 
            return m_current_state.stack.size(); 
        }
        [[nodiscard]] size_t get_current_depth() const 
        { 
            return m_current_state.current_depth; 
        }
        [[nodiscard]] size_t get_explored_count() const 
        { 
            return m_current_state.explored_count; 
        }
        [[nodiscard]] const std::vector<bool>& get_visited() const 
        { 
            return m_current_state.visited; 
        }
        [[nodiscard]] const std::vector<int>& get_distances() const 
        { 
            return m_current_state.distance; 
        }
        [[nodiscard]] const std::vector<int>& get_entry_times() const 
        { 
            return m_current_state.entry_time; 
        }
        [[nodiscard]] const std::vector<int>& get_exit_times() const 
        { 
            return m_current_state.exit_time; 
        }

    private:
        struct Graph
        {
            std::vector<std::vector<size_t>> adjacency_list;
            size_t node_count{0};
            size_t edge_count{0};
            bool is_directed{false};
            bool is_weighted{false};
            std::vector<int> node_weights;
        };

        struct DFSState
        {
            std::stack<size_t> stack;
            std::vector<bool> visited;
            std::vector<bool> in_stack;
            std::vector<size_t> parent;
            std::vector<int> distance;
            std::vector<int> entry_time;
            std::vector<int> exit_time;
            std::vector<size_t> traversal_order;
            size_t current_node{static_cast<size_t>(-1)};
            size_t current_depth{0};
            size_t explored_count{0};
            size_t comparisons{0};
            int time_counter{0};
            bool is_complete{false};
            bool target_found{false};
            size_t target_node_found{static_cast<size_t>(-1)};

            // For edge-by-edge visualization
            size_t current_neighbor_index{0};
            size_t current_neighbor{static_cast<size_t>(-1)};
            bool is_backtracking{false};

            enum class Phase
            {
                INITIALIZE,
                PUSH_START,
                POP_NODE,
                VISIT_NODE,
                EXPLORE_EDGE,
                PUSH_NEIGHBOR,
                NODE_PROCESSED,
                BACKTRACK,
                STACK_EMPTY,
                TARGET_FOUND,
                COMPLETED
            } phase{Phase::INITIALIZE};
        };

        void push_step(
            const DFSState& state, 
            const std::string& operation_id
        );
        AlgorithmStep create_step_from_state(const DFSState& state, 
            const std::string& operation_id
        ) const;
        void populate_step_metadata(AlgorithmStep& step, 
            const DFSState& state, 
            const std::string& operation_id
        ) const;
        void update_visualization_data(AlgorithmStep& step, 
            const DFSState& state, 
            const std::string& operation_id
        ) const;

        bool perform_dfs_step(DFSState& state);
        void add_neighbor_to_stack(
            DFSState& state, 
            size_t neighbor
        );

        Graph m_graph;
        size_t m_start_node{0};
        std::optional<size_t> m_target_node;
        DFSState m_current_state;

        size_t m_total_explored{0};
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_DFS_HPP