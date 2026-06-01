#ifndef CODE2LOGIC_I_SIMPLE_ALGORITHM_HPP
#define CODE2LOGIC_I_SIMPLE_ALGORITHM_HPP


#include "heuristic_type.hpp"
#include "algorithms/core/i_algorithm_metadata.hpp"
#include "algorithms/core/algorithm_observer.hpp"
#include "algorithms/core/algorithm_step.hpp"
#include "algorithms/visualizers/grid_structures.hpp"
#include "algorithms/visualizers/grid_cell_type.hpp"

#include <vector>

namespace c2l::algorithms
{
    class ISimpleAlgorithm
    {
    public:
        virtual ~ISimpleAlgorithm() = default;

        // Core algorithm interface
        virtual void initialize(const std::vector<int>& data) {};
        virtual bool step_forward() = 0;
        virtual bool step_backward() = 0;
        virtual void generate_all_steps() = 0;
        virtual void reset() = 0;
        virtual void reset_state() {}

        // Observer pattern
        virtual void add_observer(AlgorithmObserver* observer) = 0;
        virtual void remove_observer(AlgorithmObserver* observer) = 0;

        // Search algorithms support
        virtual void set_search_target([[maybe_unused]] int target) {}
        [[nodiscard]] virtual int get_search_target() const noexcept { return 0; }

        // Getters
        [[nodiscard]] virtual std::vector<int> get_original_data() const = 0;
        [[nodiscard]] virtual AlgorithmStep get_current_step() const = 0;
        [[nodiscard]] virtual size_t get_step_count() const = 0;
        [[nodiscard]] virtual size_t get_current_step_index() const = 0;
        [[nodiscard]] virtual bool is_complete() const = 0;
        [[nodiscard]] virtual bool is_steps_empty_or_invalid() const = 0;
        [[nodiscard]] virtual const std::vector<AlgorithmStep>& get_steps() const = 0;

        // Performance metrics
        [[nodiscard]] virtual int64_t get_algorithm_time_us() const = 0;
        [[nodiscard]] virtual size_t get_peak_memory_bytes() const = 0;

        // Metadata
        [[nodiscard]] virtual const IAlgorithmMetadata* metadata() const noexcept = 0;

        // Grid-based algorithm interface (optional for non-grid algorithms)
        virtual void set_grid(
            [[maybe_unused]] int rows,
            [[maybe_unused]] int cols,
            [[maybe_unused]] const std::vector<int>& grid_data)
        {}
        virtual void set_grid_cells(
            [[maybe_unused]] int rows,
            [[maybe_unused]] int cols,
            [[maybe_unused]] const std::vector<std::vector<GridCellType>>& grid)
        {}
        virtual void set_start(
            [[maybe_unused]] int row,
            [[maybe_unused]] int col)
        {}
        virtual void set_target(
            [[maybe_unused]] int row,
            [[maybe_unused]] int col)
        {}
        virtual void set_allow_diagonals(
            [[maybe_unused]] bool allow)
        {}
        virtual void set_heuristic_type(
            [[maybe_unused]] HeuristicType type)
        {}

        // Grid results getters (returns empty by default for non-grid algorithms)
        [[nodiscard]] virtual const std::vector<GridPosition>& get_path() const
        {
            static std::vector<GridPosition> empty; return empty;
        }

        [[nodiscard]] virtual const std::vector<GridPosition>& get_visited_order() const
        {
            static std::vector<GridPosition> empty; return empty;
        }

        [[nodiscard]] virtual const std::vector<GridPosition>& get_frontier_order() const
        {
            static std::vector<GridPosition> empty; return empty;
        }

        [[nodiscard]] virtual GridPosition get_current_node_grid() const
        {
            return {-1, -1};
        }

        [[nodiscard]] virtual size_t get_open_set_size() const
        {
            return 0;
        }

        [[nodiscard]] virtual size_t get_closed_set_size() const
        {
            return 0;
        }

        [[nodiscard]] virtual  float get_distance(
            [[maybe_unused]] int row,
            [[maybe_unused]] int col) const
        {
            return -1.0f;
        }


        [[nodiscard]] virtual float get_g_score(
            [[maybe_unused]] int row,
            [[maybe_unused]] int col
        ) const
        {
            return 0.0f;
        }

        [[nodiscard]] virtual float get_f_score(
            [[maybe_unused]] int row,
            [[maybe_unused]] int col
        ) const
        {
            return 0.0f;
        }
    };
} // namespace c2l::algorithms

#endif //CODE2LOGIC_I_SIMPLE_ALGORITHM_HPP
