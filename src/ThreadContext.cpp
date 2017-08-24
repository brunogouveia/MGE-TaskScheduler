#include "ThreadContext.h"

namespace MGE
{
	ThreadContext::ThreadContext()
	{

	}


	ThreadContext::~ThreadContext()
	{
		if (m_Thread)
		{
			Stop();
		}
	}

	void ThreadContext::Start(WorkerThreadFunc workerThreadFunc)
	{
		m_State = ThreadContextState::RUNNING;
		m_Thread.reset(new std::thread(workerThreadFunc, this));
	}

	void ThreadContext::Stop()
	{
		m_State = ThreadContextState::EXIT;
		if (m_Thread->joinable())
		{
			m_Thread->join();
		}
		else
		{
			m_Thread->detach();
		}

		m_Thread.reset();

		m_State = ThreadContextState::IDLE;
	}

	void ThreadContext::CreateSchedulerFiber(Fiber::FiberFunc fiberEntryPoint, void* params)
	{
		m_SchedulerFiberContext.CreateFromCurrentThreadAndRun(fiberEntryPoint, params);
	}

	void ThreadContext::SwitchToSchedulerFiber()
	{
		m_SchedulerFiberContext.fiber.SwitchToFiber();
	}

	uint32_t ThreadContext::GetThreadIndex() const
	{
		return m_ThreadIndex;
	}

	void ThreadContext::SetThreadIndex(uint32_t index)
	{
		m_ThreadIndex = index;
	}

	ThreadContextState ThreadContext::GetState() const
	{
		return m_State;
	}

	void ThreadContext::SetState(ThreadContextState state)
	{
		m_State = state;
	}

	TaskScheduler* ThreadContext::GetTaskScheduler() const
	{
		return m_TaskScheduler;
	}

	void ThreadContext::SetTaskScheduler(TaskScheduler* taskScheduler)
	{
		m_TaskScheduler = taskScheduler;
	}
}
