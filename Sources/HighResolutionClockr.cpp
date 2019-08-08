#include "HighResolutionClock.h"

HighResolutionClock::HighResolutionClock()
	: m_deltaTime(0)
	, m_totalTime(0)
{
	m_t0 = std::chrono::high_resolution_clock::now();
}

void HighResolutionClock::Tick()
{
	auto t1 = std::chrono::high_resolution_clock::now();
	m_deltaTime = std::chrono::high_resolution_clock::duration;
	m_totalTime = std::chrono::high_resolution_clock::duration;
}

void HighResolutionClock::Reset()
{
	m_t0 = std::chrono::high_resolution_clock::now();
	m_deltaTime = std::chrono::high_resolution_clock::duration;
	m_totalTime = std::chrono::high_resolution_clock::duration;
}

double HighResolutionClock::GetDeltaNanoseconds() const
{
	return m_deltaTime.count() * 1.0;
}

double HighResolutionClock::GetDeltaMicroseconds() const
{
	return m_deltaTime.count() * 1e-3;
}

double HighResolutionClock::GetDeltaMilliseconds() const
{
	return m_deltaTime.count() * 1e-6;
}

double HighResolutionClock::GetDeltaSeconds() const
{
	return m_deltaTime.count() * 1e-9;
}

double HighResolutionClock::GetTotalNanoseconds() const
{
	return m_totalTime.count() * 1.0;
}

double HighResolutionClock::GetTotalMicroseconds() const
{
	return m_totalTime.count() * 1e-3;
}

double HighResolutionClock::GetTotalMilliseconds() const
{
	return m_totalTime.count() * 1e-6;
}

double HighResolutionClock::GetTotalSeconds() const
{
	return m_totalTime.count() * 1e-9;
}