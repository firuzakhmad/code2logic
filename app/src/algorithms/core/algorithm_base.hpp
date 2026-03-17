#ifndef CODE2LOGIC_ALGORITHM_BASE_HPP
#define CODE2LOGIC_ALGORITHM_BASE_HPP

#include "algorithms/core/i_simple_algorithm.hpp"
#include"core/json_config_manager/json_config_manager.hpp"

#include <optional>

namespace c2l::algorithms
{
    class AlgorithmBase : public ISimpleAlgorithm, public Observable
    {
    public:
        AlgorithmBase() = default;
        ~AlgorithmBase() override;

    protected:

        void add_observer(AlgorithmObserver* observer) override;
        void remove_observer(AlgorithmObserver* observer) override;
        void notify_observers();
        
    private:
        void validate_data(const std::vector<int>& data) const;
        void notify_completed();
        void notify_reset();
    };

} // namespace c2l::algorithms

#endif // CODE2LOGIC_ALGORITHM_BASE_HPP