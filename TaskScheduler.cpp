#include "stdafx.h"
#include "TaskScheduler.h"

#include <assert.h>
#include <iostream>

TaskScheduler::TaskScheduler(uint32_t numWorkerThreads)
{
	// Setup fibers
	for (size_t i = 0; i < NUM_STANDARD_FIBERS; ++i)
	{
		m_StandardFiberContexts[i].CreateFiber(&TaskScheduler::FiberMainFunc, &m_StandardFiberContexts[i]);
		m_AvailableFiberContexts.push_back(&m_StandardFiberContexts[i]);
	}

	// Setup worker threads
	for (size_t i = 0; i < NUM_WORKERS_THREAD; ++i)
	{
		m_WorkerThreadContexts[i].SetThreadIndex(i);
		m_WorkerThreadContexts[i].taskScheduler = this;
		m_WorkerThreadContexts[i].Start(&TaskScheduler::WorkerThreadFunc);
	}
}


TaskScheduler::~TaskScheduler()
{
}

void TaskScheduler::RunTaskImpl(TaskDescription* tasks, uint32_t numTasks)
{
	std::lock_guard<std::mutex> guard(m_TaskQueueMutext);
	for (size_t i = 0; i < numTasks; ++i)
	{
		m_TaskDescriptionQueue.push(tasks[i]);
	}
}

void TaskScheduler::WorkerThreadFunc(void* params)
{
	assert(params && "WorkerThreadFunc being initialized with null params");
	ThreadContext& threadContext = *(static_cast<ThreadContext*>(params));

	for (int i = 0; i < NUM_WORKERS_THREAD; ++i)
	{
		std::cout << "WorkerThreadFunc: " << threadContext.GetThreadIndex() << std::endl;
	}

	threadContext.schedulerFiberContext.CreateFromCurrentThreadAndRun(&TaskScheduler::FiberSchedulerFunc, params);
}

void TaskScheduler::FiberSchedulerFunc(void* params)
{
	assert(params && "FiberSchedulerFunc being initialized with null params");
	ThreadContext& threadContext = *(static_cast<ThreadContext*>(params));

	std::cout << "FiberSchedulerFunc: " << threadContext.GetThreadIndex() << std::endl;

	int initt = threadContext.taskScheduler->m_InitializedThreads.load();
	while (!threadContext.taskScheduler->m_InitializedThreads.compare_exchange_weak(initt, initt + 1));
	while (threadContext.taskScheduler->m_InitializedThreads.load() < NUM_WORKERS_THREAD)
	{

	}

	_mm_mfence();

	while (threadContext.state != ThreadContextState::EXIT)
	{
		ExecuteNextTask(threadContext);
	}

	//threadContext.taskScheduler->m_StandardFiberContexts[threadContext.GetThreadIndex()].fiber.SwitchToFiber();
}

#include <iostream>
#include <chrono>
#include <thread>
void TaskScheduler::FiberMainFunc(void* params)
{
	assert(params && "FiberMainFunc being initialized with null params");
	FiberContext& fiberContext = *(static_cast<FiberContext*>(params));

	std::cout << "FiberMainFunc: " << std::endl;
	
	for (;;)
	{
		assert(fiberContext.taskDescription.taskEntryPoint && "Invalid taskEntryPoint");
		assert(fiberContext.taskDescription.userData && "Invalid userData");

		fiberContext.taskDescription.taskEntryPoint(fiberContext.taskDescription.userData, fiberContext);

		fiberContext.threadContext->schedulerFiberContext.fiber.SwitchToFiber();
	}
}

bool TaskScheduler::ExecuteNextTask(ThreadContext& context)
{
	TaskScheduler* taskScheduler = context.taskScheduler;
	assert(taskScheduler && "TaskScheduler invalid.");

	if (taskScheduler->m_TaskDescriptionQueue.size() == 0
		|| taskScheduler->m_AvailableFiberContexts.size() == 0)
	{
		return false;
	}

	FiberContext* fiberContext = nullptr;
	TaskDescription taskDescription;

	// Try to get a task
	{
		std::lock_guard<std::mutex> availableFibersLock(taskScheduler->m_AvailableFibersMutext);
		std::lock_guard<std::mutex> taskQueueLock(taskScheduler->m_TaskQueueMutext);

		if (taskScheduler->m_TaskDescriptionQueue.size() == 0
			|| taskScheduler->m_AvailableFiberContexts.size() == 0)
		{
			return false;
		}

		// Get next task
		fiberContext = taskScheduler->m_AvailableFiberContexts.back();
		taskScheduler->m_AvailableFiberContexts.pop_back();
		taskDescription = taskScheduler->m_TaskDescriptionQueue.back();
		taskScheduler->m_TaskDescriptionQueue.pop();
	}
	
	fiberContext->threadContext = &context;
	fiberContext->taskDescription = taskDescription;
	fiberContext->fiber.SwitchToFiber();

	std::lock_guard<std::mutex> availableFibersLock(taskScheduler->m_AvailableFibersMutext);
	taskScheduler->m_AvailableFiberContexts.push_back(fiberContext);

	return true;
}
