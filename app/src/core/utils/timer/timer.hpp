#ifndef TIMER_HPP
#define TIMER_HPP

/**
 * @brief High-resolution timing system for application and game loops
 * and performance measurement
 *
 * Features:
 * - Nanosecond precision timing
 * - Thread-safe operations
 * - Support for fixed timestep accumulation
 * - Interface-base design for testability
 * - Platform-independent high resolution clock
 */

#include "core/utils/timer/i_timer.hpp"

#include <chrono>
#include <shared_mutex>

namespace c2l::core
{
    /**
     * @class Timer
     * @brief Concrete high-resolution timer implementation
     *
     * Provides precise timing measurements suitable for application and game loops,
     * physics simulations, and performance profiling.
     */
    class Timer final : public ITimer
    {
    public:
	    /**
         * @brief Constructs a new Timer object
         *
         * Initializes all timing values to zero and captures the start time
         */
        Timer();

        Timer(const Timer&) = delete;
        Timer& operator=(const Timer&) = delete;
        Timer(Timer&&) = delete;
        Timer& operator=(Timer&&) = delete;

        /**
         * @copydoc ITimer::reset
         */
        void reset() override;

        /**
         * @copydoc ITimer::update
         */
        void update() override;

        /**
         * @copydoc ITimer::consume_accumulated_time
         */
        void consume_accumulated_time(double time) override;

        /**
         * @copydoc ITimer::get_delta_time
         */
        double get_delta_time() const noexcept override;

        /**
         * @copydoc ITimer::get_elapsed_time
         */
        double get_elapsed_time() const noexcept override;

        /**
         * @copydoc ITimer::get_accumulated_time
         */
        double get_accumulated_time() const noexcept override;

    private:
        using Clock = std::chrono::steady_clock;
        using TimePoint = std::chrono::time_point<Clock>;
        using Duration = std::chrono::duration<double>;

        TimePoint m_start_time;
        TimePoint m_last_time;
        double m_delta_time;
        double m_elapsed_time;
        double m_accumulated_time;

        mutable std::mutex m_mutex;
    };

    /**
     * @class MockTimer
     * @brief Test implementation of ITimer
     *
     * Provides predictable timing values for unit testing
     */
    class MockTimer final : public ITimer {
    public:
        void reset() override {}
        void update() override {}
        void consume_accumulated_time(double) override {}
        double get_delta_time() const noexcept override { return 0.016667; } // 60 FPS
        double get_elapsed_time() const noexcept override { return 0.0; }
        double get_accumulated_time() const noexcept override { return 0.0; }
    };

} // namespace c2l::core

#endif // TIMER_HPP