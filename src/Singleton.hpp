#pragma once

#include <atomic>
#include <mutex>

template<class T>
class Singleton
{
protected:
	static std::mutex m_Mutex;
	static std::atomic<T*> m_Instance;

public:
	Singleton()
	{ }
	virtual ~Singleton()
	{ }

	inline static T *Get()
	{
		if (m_Instance == nullptr)
		{
			std::lock_guard<std::mutex> lock(m_Mutex);
			if (m_Instance == nullptr)
				m_Instance = new T;
		}
		return m_Instance;
	}

	inline static void Destroy()
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		if (m_Instance != nullptr)
		{
			delete m_Instance.exchange(nullptr, std::memory_order_relaxed);
		}
	}
};

template <class T>
std::mutex Singleton<T>::m_Mutex;
template <class T>
std::atomic<T*> Singleton<T>::m_Instance{ nullptr };
