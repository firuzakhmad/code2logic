//
// Created by Akhmad on 1/4/26.
//

#ifndef CODE2LOGIC_ALGORITHM_OBSERVER_HPP
#define CODE2LOGIC_ALGORITHM_OBSERVER_HPP

#include <cstddef>

namespace c2l::algorithms
{
    class AlgorithmObserver
    {
    public:
        virtual ~AlgorithmObserver() = default;
        virtual void on_step_changed() = 0;
    };
} // namespace c2l::algorithms

#endif //CODE2LOGIC_ALGORITHM_OBSERVER_HPP