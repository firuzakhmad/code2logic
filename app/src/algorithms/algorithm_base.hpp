#ifndef CODE2LOGIC_ALGORITHM_BASE_HPP
#define CODE2LOGIC_ALGORITHM_BASE_HPP

#include "i_simple_algorithm.hpp"
#include "algorithm_types.hpp"
#include "core/utils/logger/logger.hpp"
#include "algorithms/algorithm_step.hpp"

#include <optional>

namespace c2l::algorithms
{
    class AlgorithmBase : public ISimpleAlgorithm
    {
    public:
        ~AlgorithmBase() override = default;

        [[nodiscard]] std::string get_name() const final;
        [[nodiscard]] AlgorithmCategory get_category() const final;

        [[nodiscard]] std::string get_description() const override;
        [[nodiscard]] AlgorithmStep get_current_step() const override;

        // Common default implementations
        [[nodiscard]] bool supports_backward_steps() const override     { return true; }
        [[nodiscard]] bool supports_random_access() const override      { return false; }
        [[nodiscard]] bool requires_specialized_data() const override   { return false; }

    protected:
        void validate_data(const std::vector<int>& data) const;

        void add_observer(AlgorithmObserver* observer) override;
        void remove_observer(AlgorithmObserver* observer) override;
        void notify_observers() const;


        std::vector<int> m_original_data;
        size_t m_current_step_index     {0};

        std::vector<AlgorithmObserver*> m_observers;

        mutable std::optional<AlgorithmStep> m_cached_step;
        mutable size_t m_cached_step_index   {std::numeric_limits<size_t>::max()};
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_ALGORITHM_BASE_HPP