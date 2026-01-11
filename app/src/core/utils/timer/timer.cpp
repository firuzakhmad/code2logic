#include "timer.hpp"

namespace c2l::core
{
	Timer::Timer()
		: m_start_time{Clock::now()},
		  m_last_time{m_start_time},
		  m_delta_time{0.0},
		  m_elapsed_time{0.0},
		  m_accumulated_time{0.0}
	{}

	void Timer::reset()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_start_time = m_last_time = Clock::now();

	    m_delta_time		= 0.0;
	    m_elapsed_time		= 0.0;
	    m_accumulated_time	= 0.0;
	}

	void Timer::update()
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		const auto now = Clock::now();
		const Duration delta = now - m_last_time;

		// Updating timing values using relaxed memory ordering.
		m_delta_time = delta.count();
		m_elapsed_time += delta.count();
		m_accumulated_time += delta.count();

		m_last_time = now;
	}

	void Timer::consume_accumulated_time(const double time)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_accumulated_time = std::max(0.0, m_accumulated_time - time);
	}

	double Timer::get_delta_time() const noexcept
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_delta_time;
	}

	double Timer::get_elapsed_time() const noexcept
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_elapsed_time;
	}

	double Timer::get_accumulated_time() const noexcept
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_accumulated_time;
	}

} // namespace c2l::core