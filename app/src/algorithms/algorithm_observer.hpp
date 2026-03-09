//
// Created by Akhmad on 1/4/26.
//

#ifndef CODE2LOGIC_ALGORITHM_OBSERVER_HPP
#define CODE2LOGIC_ALGORITHM_OBSERVER_HPP

#include <cstddef>
#include <mutex>
#include <atomic>
#include <algorithm>

namespace c2l::algorithms
{
    class AlgorithmObserver
    {
    public:
        virtual ~AlgorithmObserver() = default;
        virtual void on_step_changed() = 0;
        virtual void on_algorithm_completed() {}
        virtual void on_algorithm_reset() {}
    };


    class Observable
    {
    private:
        std::vector<AlgorithmObserver*> m_observers;
        mutable std::recursive_mutex m_observer_mutex; 
        std::atomic<bool> m_notifying{false};

    public:
        virtual ~Observable()
        {
            // Clear observers without notifying during destruction
            std::lock_guard<std::recursive_mutex> lock(m_observer_mutex);
            m_observers.clear();
        }

        void add_observer(AlgorithmObserver* observer)
        {
            if (!observer) return;
            
            std::lock_guard<std::recursive_mutex> lock(m_observer_mutex);
            if (std::find(m_observers.begin(), m_observers.end(), observer) == m_observers.end())
            {
                m_observers.push_back(observer);
            }
        }

        void remove_observer(AlgorithmObserver* observer)
        {
            if (!observer || m_notifying) return; 
            
            std::lock_guard<std::recursive_mutex> lock(m_observer_mutex);
            m_observers.erase(
                std::remove(m_observers.begin(), m_observers.end(), observer),
                m_observers.end()
            );
        }

        void remove_all_observers()
        {
            if (m_notifying) return; 
            
            std::lock_guard<std::recursive_mutex> lock(m_observer_mutex);
            m_observers.clear();
        }

    protected:
        void notify_observers()
        {
            std::lock_guard<std::recursive_mutex> lock(m_observer_mutex);
            
            m_notifying = true;
            
            auto observers_copy = m_observers;
            
            for (auto* observer : observers_copy)
            {
                if (observer)
                {
                    observer->on_step_changed();
                }
            }
            
            m_notifying = false;
        }

        void notify_completed()
        {
            std::lock_guard<std::recursive_mutex> lock(m_observer_mutex);
            m_notifying = true;
            
            auto observers_copy = m_observers;
            for (auto* observer : observers_copy)
            {
                if (observer)
                {
                    observer->on_algorithm_completed();
                }
            }
            
            m_notifying = false;
        }

        void notify_reset()
        {
            std::lock_guard<std::recursive_mutex> lock(m_observer_mutex);
            m_notifying = true;
            
            auto observers_copy = m_observers;
            for (auto* observer : observers_copy)
            {
                if (observer)
                {
                    observer->on_algorithm_reset();
                }
            }
            
            m_notifying = false;
        }

        [[nodiscard]] size_t get_observer_count() const
        {
            std::lock_guard<std::recursive_mutex> lock(m_observer_mutex);
            return m_observers.size();
        }
    };
} // namespace c2l::algorithms

#endif //CODE2LOGIC_ALGORITHM_OBSERVER_HPP