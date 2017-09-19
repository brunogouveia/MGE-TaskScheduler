#include "FiberContext.h"

#include "TaskScheduler.h"

namespace MGE
{
	FiberContext::FiberContext()
	{
	}


	FiberContext::~FiberContext()
	{
	}

	void FiberContext::CreateFiber(FiberFunc fiberFunc, void* parameters)
	{
		m_Fiber.CreateFiber(fiberFunc, parameters);
	}

	void FiberContext::CreateFromCurrentThreadAndRun(FiberFunc fiberFunc, void* params)
	{
		m_Fiber.CreateFromCurrentThreadAndRun(fiberFunc, params);
	}

	void FiberContext::ReadyToWakeup()
	{
		if (m_TaskCounter)
		{
			m_TaskCounter->Decrement();
		}
	}

	void FiberContext::WaitForCounterAndFree(std::shared_ptr<TaskCounter> taskCounter, uint32_t value)
	{
		ThreadContext* threadContext = GetThreadContext();
		TaskScheduler* taskScheduler = threadContext->GetTaskScheduler();
		m_TaskCounter = taskCounter;

		m_State = FiberContextState::YIELD;
		taskScheduler->WaitForCounterAndFree(*this, taskCounter.get(), value);

		// Switch back to scheduler fiber
		Fiber::SwitchTo(m_Fiber, threadContext->GetShedulerFiber());
	}

	Fiber& FiberContext::GetFiber()
	{
		return m_Fiber;
	}

	FiberContextState FiberContext::GetState() const
	{
		return m_State.load();
	}

	void FiberContext::SetState(FiberContextState state)
	{
		m_State.store(state);
	}

	ThreadContext* FiberContext::GetThreadContext() const
	{
		return m_ThreadContext;
	}

	void FiberContext::SetThreadContext(ThreadContext* threadContext)
	{
		m_ThreadContext = threadContext;
	}

	void FiberContext::RunTaskImpl(TaskList& taskList)
	{
		TaskScheduler* taskScheduler = GetThreadContext()->GetTaskScheduler();
		taskScheduler->RunTaskImpl(taskList);
	}
}