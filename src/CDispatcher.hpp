#pragma once

#include "CSingleton.hpp"

#include <queue>
#include <mutex>

#include "types.hpp"


class CDispatcher : public CSingleton < CDispatcher >
{
	friend class CSingleton < CDispatcher >;
public: //type definitions
	using Function_t = std::function <void()>;

private: //constructor / destructor
	CDispatcher() = default;
	~CDispatcher() = default;

private: //variables
	std::queue<Function_t> m_Queue;
	std::mutex m_QueueMtx;

public: //functions
	void Dispatch(Function_t &&func);
	void Process();

};
