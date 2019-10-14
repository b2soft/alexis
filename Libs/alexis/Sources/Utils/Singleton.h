#pragma once

#include <memory>

namespace alexis
{
	template<typename T>
	class Singleton
	{
	public:
		static T* GetInstance();
		static void DestroyInstance();
		static bool HasInstance();

	private:

		Singleton(Singleton const&) = delete;
		Singleton& operator=(Singleton const&)
		{
		}

	protected:
		static T* m_instance;

		Singleton()
		{
			m_instance = static_cast <T*> (this);
		};
		~Singleton()
		{
		}
	};

	template<typename T>
	typename T* Singleton<T>::m_instance = 0;

	template<typename T>
	T* Singleton<T>::GetInstance()
	{
		if (!m_instance)
		{
			Singleton<T>::m_instance = new T();
		}

		return m_instance;
	}
	template<typename T>
	bool Singleton<T>::HasInstance()
	{
		return m_instance != nullptr;
	}


	template<typename T>
	void Singleton<T>::DestroyInstance()
	{
		delete Singleton<T>::m_instance;
		Singleton<T>::m_instance = 0;
	}
}