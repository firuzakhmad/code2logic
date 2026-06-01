#ifndef CODE2LOGIC_ALGORITHM_BASE_HPP
#define CODE2LOGIC_ALGORITHM_BASE_HPP

#include "algorithms/core/i_simple_algorithm.hpp"
#include "core/json_config_manager/json_config_manager.hpp"
#include "algorithms/visualizers/grid_structures.hpp"

namespace c2l::algorithms
{
    class AlgorithmBase : public ISimpleAlgorithm, public Observable
    {
    public:
        AlgorithmBase() = default;
        ~AlgorithmBase() override;

        // ISimpleAlgorithm implementation
        void initialize(const std::vector<int>& data) override;
        bool step_forward() override;
        bool step_backward() override;
        void reset() override;

        void reset_state() override = 0;
        void generate_all_steps() override = 0;

        void add_observer(AlgorithmObserver* observer) override;
        void remove_observer(AlgorithmObserver* observer) override;

        [[nodiscard]] std::vector<int> get_original_data() const override;
        [[nodiscard]] AlgorithmStep get_current_step() const override;
        [[nodiscard]] size_t get_step_count() const override;
        [[nodiscard]] size_t get_current_step_index() const override;
        [[nodiscard]] bool is_complete() const override;
        [[nodiscard]] bool is_steps_empty_or_invalid() const override;
        [[nodiscard]] const std::vector<AlgorithmStep>& get_steps() const override;
        [[nodiscard]] int64_t get_algorithm_time_us() const override;
        [[nodiscard]] size_t get_peak_memory_bytes() const override;

    protected:
        void notify_observers();
        void notify_reset();
        void notify_completed();

        // Common members for all algorithms
        std::vector<int> m_original_data;
        std::vector<AlgorithmStep> m_steps;
        size_t m_current_step_index{0};

        // Performance metrics
        int64_t m_algorithm_time_us{0};
        size_t m_peak_memory_bytes{0};

    private:
        void validate_data(const std::vector<int>& data) const;
    };
} // namespace c2l::algorithms

#endif // CODE2LOGIC_ALGORITHM_BASE_HPP