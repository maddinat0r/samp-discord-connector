#pragma once

#include "Singleton.hpp"

#include <queue>
#include <mutex>
#include <functional>

#include "types.hpp"


class PawnDispatcher : public Singleton<PawnDispatcher>
{
	friend class Singleton<PawnDispatcher>;
public: //type definitions
	using Function_t = std::function<void()>;

private: //constructor / destructor
	PawnDispatcher() = default;
	~PawnDispatcher() = default;

private: //variables
	std::queue<Function_t> m_Queue;
	std::mutex m_QueueMtx;

public: //functions
	void Dispatch(Function_t &&func);
	void Process();

};
