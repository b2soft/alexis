
namespace alexis
{
	class Timer
	{
	public:
		Timer();

		float AppTime() const; //Get App Time in seconds
		float DeltaTime() const; //Get Delta Time in seconds
		
		void Reset();
		void Start();
		void Stop();
		void Tick();

	private:
		double m_secondsPerCount{ 0.0 };
		double m_deltaTime{ -1.0 };

		__int64 m_baseTime{ 0 };
		__int64 m_pausedTime{ 0 };
		__int64 m_stopTime{ 0 };
		__int64 m_prevTime{ 0 };
		__int64 m_currTime{ 0 };

		bool m_stopped{ false };
	};
}