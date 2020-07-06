#include "Precompiled.h"

#include "Timer.h"

namespace alexis
{

	Timer::Timer()
	{
		__int64 countsPerSec = 0;
		QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
		m_secondsPerCount = 1.0 / (double)countsPerSec;
	}

	float Timer::AppTime() const
	{
		if (m_stopped)
		{
			return static_cast<float>(((m_stopTime - m_pausedTime) - m_baseTime) * m_secondsPerCount);
		}

		return static_cast<float>(((m_currTime - m_pausedTime) - m_baseTime) * m_secondsPerCount);
	}

	float Timer::DeltaTime() const
	{
		return static_cast<float>(m_deltaTime);
	}

	void Timer::Reset()
	{
		__int64 currTime = 0;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

		m_baseTime = currTime;
		m_prevTime = currTime;
		m_stopTime = 0;
		m_stopped = false;
	}

	void Timer::Start()
	{
		__int64 startTime = 0;
		QueryPerformanceCounter((LARGE_INTEGER*)&startTime);

		if (m_stopped)
		{
			m_pausedTime += (startTime - m_stopTime);

			m_prevTime = startTime;

			m_stopTime = 0;
			m_stopped = false;
		}
	}

	void Timer::Stop()
	{
		if (!m_stopped)
		{
			__int64 currTime = 0;
			QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

			m_stopTime = currTime;
			m_stopped = true;
		}
	}

	void Timer::Tick()
	{
		if (m_stopped)
		{
			m_deltaTime = 0.0;
			return;
		}

		// Get the current time
		__int64 currTime = 0;
		QueryPerformanceCounter((LARGE_INTEGER*)&currTime);
		m_currTime = currTime;

		// Time difference
		m_deltaTime = (m_currTime - m_prevTime) * m_secondsPerCount;

		m_prevTime = m_currTime;

		// In case CPU goes to Power save or process is shuffled to another CPU, deltaTime can be negative
		// So force it to be non-negative
		if (m_deltaTime < 0.0)
		{
			m_deltaTime = 0.0;
		}
	}

}