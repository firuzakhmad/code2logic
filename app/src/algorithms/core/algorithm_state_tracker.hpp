//
// Created by Akhmad on 1/2/26.
//

#ifndef CODE2LOGIC_ALGORITHM_STATE_TRACKER_HPP
#define CODE2LOGIC_ALGORITHM_STATE_TRACKER_HPP

#include <cstddef>
#include <vector>
#include <string>

namespace c2l::algorithms
{
    template<typename T>
    class AlgorithmStateTracker
    {
    public:
        AlgorithmStateTracker() = default;
        explicit AlgorithmStateTracker(const T& initial_state)
            : m_current_state(initial_state)
        {}

        void update(const T& new_state)
        {
            m_previous_state = m_current_state;
            m_current_state = new_state;
            m_change_count++;
        }

        [[nodiscard]] const T& current() const noexcept     { return m_current_state; }
        [[nodiscard]] const T& previous() const noexcept    { return m_previous_state; }
        [[nodiscard]] bool has_changed() const noexcept     { return m_change_count > 0; }
        [[nodiscard]] size_t change_count() const noexcept  { return m_change_count; }

        // Extract changes for metadata
        [[nodiscard]] std::vector<std::pair<std::string, std::string>> get_changes() const;

    private:
        T m_current_state       {};
        T m_previous_state      {};
        size_t m_change_count   {0};
    };
} // namespace c2l::algorithms

#endif //CODE2LOGIC_ALGORITHM_STATE_TRACKER_HPP