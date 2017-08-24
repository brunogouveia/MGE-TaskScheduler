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

	void FiberContext::CreateFiber(FiberEntryPoint fiberEntryPoint, void* parameters)
	{
		fiber.CreateFiber(fiberEntryPoint, parameters);
	}

	void FiberContext::CreateFromCurrentThreadAndRun(FiberEntryPoint fiberEntryPoint, void* params)
	{
		fiber.CreateFromCurrentThreadAndRun(fiberEntryPoint, params);
	}

	void FiberContext::WaitForCounterAndFree(TaskCounter* taskCounter, uint32_t value)
	{
		TaskScheduler* taskScheduler = m_ThreadContext->GetTaskScheduler();

		taskScheduler->WaitForCounterAndFree(*this, taskCounter, value);
		state = FiberContextState::WAITING;

		// Switch back to scheduler fiber
		m_ThreadContext->SwitchToSchedulerFiber();
	}

	ThreadContext* FiberContext::GetThreadContext() const
	{
		return m_ThreadContext;
	}

	void FiberContext::SetThreadContext(ThreadContext* threadContext)
	{
		m_ThreadContext = threadContext;
	}

	void FiberContext::RunTask(TaskDescription* tasks, uint32_t numTasks, TaskCounter* taskCounter)
	{
		TaskScheduler* taskScheduler = m_ThreadContext->GetTaskScheduler();
		taskScheduler->RunTasks(tasks, numTasks, taskCounter);
	}
}