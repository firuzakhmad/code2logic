//
// Created by Akhmad on 1/5/26.
//

#include "algorithms/algorithm_base.hpp"

#include <algorithm>

namespace c2l::algorithms
{
    std::string AlgorithmBase::get_name() const
    {
        return algorithm_type_to_string(get_type());
    }

    AlgorithmCategory AlgorithmBase::get_category() const
    {
        return get_algorithm_category(get_type());
    }

    std::string AlgorithmBase::get_description() const
    {
        return "No description available";
    }

    AlgorithmStep AlgorithmBase::get_current_step() const {
        if (is_steps_empty_or_invalid()) return {};

        // Centralized caching logic
        if (!m_cached_step || m_cached_step_index != get_current_step_index()) {
            m_cached_step = AlgorithmStep{};
            m_cached_step_index = get_current_step_index();

            // Calling a specialized "worker" function implemented by children
            populate_step_metadata(*m_cached_step);
        }
        return *m_cached_step;
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
        m_observers.push_back(observer);
    }

    void AlgorithmBase::remove_observer(AlgorithmObserver* observer)
    {
        const auto it = std::find(m_observers.begin(), m_observers.end(), observer);
        if (it != m_observers.end())
            m_observers.erase(it);
    }

    void AlgorithmBase::notify_observers() const
    {
        for (auto* obs : m_observers)
        {
            if (obs) obs->on_step_changed();
        }
    }

} // namespace c2l::algorithms