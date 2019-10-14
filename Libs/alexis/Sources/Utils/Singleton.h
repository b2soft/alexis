#pragma once

#include <memory>

namespace alexis
{
	template<class T>
	class Singleton
	{
	public:
		Singleton() = delete;
		Singleton(const Singleton&) = delete;
		Singleton& operator=(const Singleton&) = delete;
		Singleton(Singleton&&) = default;
		Singleton& operator=(Singleton&&) = default;

		static T* Get();
		static void Destroy();
		static bool HasInstance();
		static void Create();

	private:
		static std::unique_ptr<T> m_instance;
	};

	template<class T>
	void alexis::Singleton<T>::Destroy()
	{
		m_instance.reset();
	}

	template<class T>
	void alexis::Singleton<T>::Create()
	{
		m_instance = std::make_unique<T>();
	}

	template<class T>
	bool alexis::Singleton<T>::HasInstance()
	{
		return m_instance != nullptr;
	}

	template<class T>
	T* alexis::Singleton<T>::Get()
	{
		return m_instance.get();
	}

	template<class T>
	std::unique_ptr<T> alexis::Singleton<T>::m_instance = nullptr;
}