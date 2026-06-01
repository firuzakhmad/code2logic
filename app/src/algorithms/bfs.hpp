#ifndef CODE2LOGIC_BFS_HPP
#define CODE2LOGIC_BFS_HPP

#include "algorithms/core/json_algorithm_base.hpp"
#include "algorithms/core/algorithm_step.hpp"
#include <queue>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <optional>

namespace c2l::algorithms
{
    /**
     * @brief Breadth-First Search algorithm with JSON-driven metadata
     *
     * BFS traverses a graph level by level, exploring all neighbors
     * at the current depth before moving to the next level.
     */
    class BFS final : public JsonAlgorithmBase
    {
    public:
        explicit BFS(core::JsonConfigManager& json_config_manager);
        ~BFS() override = default;

        BFS(const BFS&) = delete;
        BFS& operator=(const BFS&) = delete;
        BFS(BFS&&) noexcept = delete;
        BFS& operator=(BFS&&) noexcept = delete;

        void generate_all_steps() override;
        void reset_state() override;

        void set_graph_structure(
            const std::vector<std::vector<size_t>>& adjacency_list
        );
        void set_start_node(size_t start);
        void set_target_node(
            std::optional<size_t> target = std::nullopt
        );

        // Get current BFS state for visualization
        [[nodiscard]] size_t get_current_node() const 
        { 
            return m_current_state.current_node; 
        }
        [[nodiscard]] size_t get_queue_size() const 
        { 
            return m_current_state.queue.size(); 
        }
        [[nodiscard]] size_t get_current_level() const 
        { 
            return m_current_state.current_level; 
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

        struct BFSState
        {
            std::queue<size_t> queue;
            std::vector<bool> visited;
            std::vector<bool> in_queue;
            std::vector<size_t> parent;
            std::vector<int> distance;
            std::vector<size_t> traversal_order;
            size_t current_node{static_cast<size_t>(-1)};
            size_t current_level{0};
            size_t visited_count{0};
            size_t explored_count{0};
            size_t comparisons{0};
            bool is_complete{false};
            bool target_found{false};
            size_t target_node_found{static_cast<size_t>(-1)};

            // For edge-by-edge visualization
            size_t current_neighbor_index{0};
            size_t current_neighbor{static_cast<size_t>(-1)};

            enum class Phase
            {
                INITIALIZE,
                ENQUEUE_START,
                DEQUEUE_NODE,
                VISIT_NODE,
                EXPLORE_EDGE,
                ENQUEUE_NEIGHBOR,
                NODE_PROCESSED,
                QUEUE_EMPTY,
                TARGET_FOUND,
                COMPLETED
            } phase{Phase::INITIALIZE};
        };

        void push_step(
            const BFSState& state, 
            const std::string& operation_id
        );
        AlgorithmStep create_step_from_state(
            const BFSState& state, 
            const std::string& operation_id
        ) const;
        void populate_step_metadata(
            AlgorithmStep& step, 
            const BFSState& state, 
            const std::string& operation_id
        ) const;
        void update_visualization_data(
            AlgorithmStep& step, 
            const BFSState& state, 
            const std::string& operation_id
        ) const;

        bool perform_bfs_step(BFSState& state);
        void add_neighbor_to_queue(
            BFSState& state, 
            size_t neighbor
        );

        Graph m_graph;
        std::optional<size_t> m_start_node{0};
        std::optional<size_t> m_target_node;
        BFSState m_current_state;
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_BFS_HPP