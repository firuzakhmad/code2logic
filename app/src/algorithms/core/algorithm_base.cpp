//
// Created by Akhmad on 1/5/26.
//

#include "algorithms/core/algorithm_base.hpp"

#include <algorithm>

namespace c2l::algorithms
{
            
        
    AlgorithmBase::~AlgorithmBase()
    {
        remove_all_observers();
    }

    void AlgorithmBase::validate_data(const std::vector<int>& data) const
    {
        if (data.empty())
        {
            LOG_ERROR("Algorithm cannot be initialized with empty data");
        }
    }

    void AlgorithmBase::add_observer(AlgorithmObserver* observer)
    {
        Observable::add_observer(observer);
    }

    void AlgorithmBase::remove_observer(AlgorithmObserver* observer)
    {
        Observable::remove_observer(observer);

    }

    void AlgorithmBase::notify_observers()
    {
        Observable::notify_observers();
    } 

    void AlgorithmBase::notify_completed()
    {
        Observable::notify_completed();
    }

    void AlgorithmBase::notify_reset()
    {
        Observable::notify_reset();
    }

} // namespace c2l::algorithms