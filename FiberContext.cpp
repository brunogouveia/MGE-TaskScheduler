#include "stdafx.h"
#include "FiberContext.h"

#include "TaskScheduler.h"

FiberContext::FiberContext()
{
}


FiberContext::~FiberContext()
{
}

void FiberContext::CreateFiber(FiberFunc fiberFunc, void* parameters)
{
	fiber.CreateFiber(fiberFunc, parameters);
}

void FiberContext::CreateFromCurrentThreadAndRun(FiberFunc fiberFunc, void* params)
{
	fiber.CreateFromCurrentThreadAndRun(fiberFunc, params);
}

void FiberContext::WaitForCounterAndFree(TaskCounter& taskCounter, uint32_t value)
{
	TaskScheduler* taskScheduler = threadContext->taskScheduler;

	taskScheduler->WaitForCounterAndFree(*this, taskCounter, value);
	state = FiberContextState::WAITING;

	// Switch back to scheduler fiber
	threadContext->schedulerFiberContext.fiber.SwitchToFiber();
}

void FiberContext::RunTaskImpl(TaskDescription* tasks, uint32_t numTasks, TaskCounter* taskCounter)
{
	TaskScheduler* taskScheduler = threadContext->taskScheduler;
	taskScheduler->RunTaskImpl(tasks, numTasks, taskCounter);
}