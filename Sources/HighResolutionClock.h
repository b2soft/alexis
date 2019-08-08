#pragma once

#include <chrono>

class HighResolutionClock
{
public:
	HighResolutionClock();

	// Tick the high resolution clock.
	// Tick the clock before reading the delta time for the first time.
	// Only tick the clock once per frame.
	// Use the Get* functions to return the elapsed time between ticks.
	void Tick();

	// Reset the clock
	void Reset();

	double GetDeltaNanoseconds() const;
	double GetDeltaMicroseconds() const;
	double GetDeltaMilliseconds() const;
	double GetDeltaSeconds() const;

	double GetTotalNanoseconds() const;
	double GetTotalMicroseconds() const;
	double GetTotalMilliseconds() const;
	double GetTotalSeconds() const;

private:
	// Initial time point
	std::chrono::high_resolution_clock::time_point m_t0;

	// Time since last tick
	std::chrono::high_resolution_clock::duration m_deltaTime;
	std::chrono::high_resolution_clock::duration m_totalTime;
};

HighResolutionClock::HighResolutionClock()
{

}

void HighResolutionClock::Tick()
{

}

void HighResolutionClock::Reset()
{

}

double HighResolutionClock::GetDeltaNanoseconds() const
{

}

double HighResolutionClock::GetDeltaMicroseconds() const
{

}

double HighResolutionClock::GetDeltaMilliseconds() const
{

}

double HighResolutionClock::GetDeltaSeconds() const
{

}

double HighResolutionClock::GetTotalNanoseconds() const
{

}

double HighResolutionClock::GetTotalMicroseconds() const
{

}

double HighResolutionClock::GetTotalMilliseconds() const
{

}

double HighResolutionClock::GetTotalSeconds() const
{

}
