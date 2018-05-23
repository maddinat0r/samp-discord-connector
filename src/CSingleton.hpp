#pragma once

#include <atomic>
#include <mutex>

template<class T>
class CSingleton
{
protected:
	static std::mutex m_Mutex;
	static std::atomic<T*> m_Instance;

public:
	CSingleton()
	{ }
	virtual ~CSingleton()
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
			delete m_Instance;
			m_Instance = nullptr;
		}
	}
};

template <class T>
std::mutex CSingleton<T>::m_Mutex;
template <class T>
std::atomic<T*> CSingleton<T>::m_Instance{ nullptr };
