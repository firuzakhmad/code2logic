#ifndef CODE2LOGIC_I_TIMER_HPP
#define CODE2LOGIC_I_TIMER_HPP

namespace c2l::core
{
    /**
     * @class ITimer
     * @brief Abstract interface for timing functionality
     *
     * Provides a contract for timing implementation enabling dependency injection
     */
    class ITimer
    {
    public:
        virtual ~ITimer() = default;

        /**
         * @brief Reset all timer values to zero
         */
        virtual void reset() = 0;

        /**
         * @brief Update timer with current frame time
         *
         * Should be called once per frame to update delta time calculations
         */
        virtual void update() = 0;

        /**
         * @brief Consume accumulated time for fixed timestep simulations
         * @param time Amount of time to consume from the accumulator
         */
        virtual void consume_accumulated_time(double time) = 0;

        /**
         * @brief Get time since last update (in seconds)
         * @return Delta time in seconds with nanosecond precision
         */
        virtual double get_delta_time() const noexcept = 0;

        /**
         * @brief Get total elapsed time since last reset (in seconds)
         * @return Elapsed time in seconds with nanosecond precision
         */
        virtual double get_elapsed_time() const noexcept = 0;

        /**
         * @brief Get accumulated time for fixed timestep simulations
         * @return Accumulated time in seconds
         */
        virtual double get_accumulated_time() const noexcept = 0;
    };

} // namespace c2l::core

#endif //CODE2LOGIC_I_TIMER_HPP