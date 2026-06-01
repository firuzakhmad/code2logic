//
// Created by Akhmad on 1/5/26.
//

#include "algorithms/core/algorithm_base.hpp"
#include "core/utils/variables.hpp"

#include <algorithm>
#include <chrono>

namespace c2l::algorithms
{
    AlgorithmBase::~AlgorithmBase()
    {
        remove_all_observers();
    }

    void AlgorithmBase::initialize(const std::vector<int>& data)
    {
        reset();
        m_original_data = data;

        // Time the actual algorithm work, not playback
        const auto t_start = std::chrono::high_resolution_clock::now();
        generate_all_steps();
        const auto t_end   = std::chrono::high_resolution_clock::now();

        m_algorithm_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
            t_end - t_start).count();

        // Memory = number of steps × size of one step (dominant allocation)
        m_peak_memory_bytes = m_steps.size() * sizeof(AlgorithmStep);

        notify_observers();
    }

    bool AlgorithmBase::step_forward()
    {
        if (!m_steps.empty() && m_current_step_index < m_steps.size() - 1)
        {
            m_current_step_index++;
            notify_observers();
            return true;
        }
        return false;
    }

    bool AlgorithmBase::step_backward()
    {
        if (m_current_step_index > 0)
        {
            m_current_step_index--;
            notify_observers();
            return true;
        }
        return false;
    }

    void AlgorithmBase::reset()
    {
        m_current_step_index = 0;
        m_steps.clear();
        reset_state();
    }

    std::vector<int> AlgorithmBase::get_original_data() const
    {
        return m_original_data;
    }

    AlgorithmStep AlgorithmBase::get_current_step() const
    {
        if (m_steps.empty() || m_current_step_index >= m_steps.size())
        {
            return AlgorithmStep{};
        }
        return m_steps[m_current_step_index];
    }

    size_t AlgorithmBase::get_step_count() const
    {
        return m_steps.size();
    }

    size_t AlgorithmBase::get_current_step_index() const
    {
        return m_current_step_index;
    }

    bool AlgorithmBase::is_complete() const
    {
        return m_steps.empty() || m_current_step_index >= m_steps.size() - 1;
    }

    bool AlgorithmBase::is_steps_empty_or_invalid() const
    {
        return m_steps.empty() || m_current_step_index >= m_steps.size();
    }


    void AlgorithmBase::validate_data(const std::vector<int>& data) const
    {
        if (data.empty())
        {
            LOG_ERROR("Algorithm cannot be initialized with empty data");
        }
    }

    const std::vector<AlgorithmStep>& AlgorithmBase::get_steps() const
    {
        return m_steps;
    }
    [[nodiscard]] int64_t AlgorithmBase::get_algorithm_time_us()  const
    {
        return m_algorithm_time_us;
    }
    [[nodiscard]] size_t  AlgorithmBase::get_peak_memory_bytes()  const
    {
        return m_peak_memory_bytes;
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