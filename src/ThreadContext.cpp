#include "ThreadContext.h"

namespace MGE
{
	ThreadContext::ThreadContext()
	{
	}

	ThreadContext::~ThreadContext()
	{
		if (m_Thread == nullptr)
		{
			return;
		}

		if (m_Thread->joinable())
		{
			m_Thread->join();
		}
		else
		{
			m_Thread->detach();
		}
	}

	void ThreadContext::Start(WorkerThreadFunc workerThreadFunc)
	{
		m_State = ThreadContextState::RUNNING;
		m_Thread.reset(new std::thread(workerThreadFunc, this));
	}

	void ThreadContext::Stop()
	{
		m_Thread.reset();
	}

	uint32_t ThreadContext::GetThreadIndex() const
	{
		return m_ThreadIndex;
	}

	void ThreadContext::SetThreadIndex(uint32_t index)
	{
		m_ThreadIndex = index;
	}

	Fiber& ThreadContext::GetShedulerFiber()
	{
		return m_SchedulerFiberContext.GetFiber();
	}

	TaskScheduler* ThreadContext::GetTaskScheduler() const
	{
		return m_TaskScheduler;
	}

	void ThreadContext::SetTaskScheduler(TaskScheduler * taskScheduler)
	{
		m_TaskScheduler = taskScheduler;
	}

	ThreadContextState ThreadContext::GetState() const
	{
		return m_State.load(std::memory_order_relaxed);
	}

	void ThreadContext::SetState(ThreadContextState state)
	{
		m_State = state;
	}
}
