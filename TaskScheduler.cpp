#include "stdafx.h"
#include "TaskScheduler.h"

#include <assert.h>
#include <iostream>

__declspec(thread) uint32_t m_IsWorkerThread = 0;

TaskScheduler::TaskScheduler()
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
		m_WorkerThreadContexts[i].SetTaskScheduler(this);
		m_WorkerThreadContexts[i].Start(&TaskScheduler::WorkerThreadFunc);
	}
}


TaskScheduler::~TaskScheduler()
{
	// Shutdown worker threads
	for (size_t i = 0; i < NUM_WORKERS_THREAD; ++i)
	{
		m_WorkerThreadContexts[i].SetState(ThreadContextState::EXIT);
	}

	for (size_t i = 0; i < NUM_WORKERS_THREAD; ++i)
	{
		m_WorkerThreadContexts[i].Stop();
	}
}

void TaskScheduler::WaitForCounterAndFree(FiberContext& fiberContext, TaskCounter* counter, uint32_t value)
{
	assert(counter->onCompleteAction == nullptr, "Overriding counter.onCompleteAction function.");

	// TODO - DELETE COUNTER

	// Set complete action to move fiber to wakeup list.
	counter->onCompleteAction = [this, &fiberContext]()
	{
		MoveFiberToWakeupList(fiberContext);
	};

	// Put Fiber to wait list.
	MoveFiberToWaitList(fiberContext);
}

void TaskScheduler::WaitForCounterAndFree(TaskCounter* counter, uint32_t value)
{
	assert(counter->onCompleteAction == nullptr, "Overriding counter.onCompleteAction function.");

	static std::mutex schedulerThreadMutex;
	static std::condition_variable schedulerThreadCV;
	static bool doneWaiting;

	doneWaiting = false;

	counter->onCompleteAction = [&]()
	{
		std::unique_lock<std::mutex> lock(schedulerThreadMutex);
		doneWaiting = true;
		schedulerThreadCV.notify_one();
	};

	std::unique_lock<std::mutex> lock(schedulerThreadMutex);
	schedulerThreadCV.wait(lock, [&]() -> bool { return doneWaiting; });
}

void TaskScheduler::WaitAllTasks()
{
	std::unique_lock<std::mutex> lock(m_AllTasksFinishedMutex);
	while (m_NumPendingTasks > 0)
	{
		m_AllTasksFinished.wait(lock);
	}
}

void TaskScheduler::RunTaskImpl(TaskDescription* tasks, uint32_t numTasks, TaskCounter* taskCounter)
{
	// Add the tasks to the queue
	{
		std::lock_guard<std::mutex> guard(m_TaskQueueMutext);
		for (size_t i = 0; i < numTasks; ++i)
		{
			tasks[i].counter = taskCounter;
			m_TaskDescriptionQueue.push(tasks[i]);
		}
	}

	// Increment the num of pending tasks
	{
		std::lock_guard<std::mutex> lock(m_AllTasksFinishedMutex);
		m_NumPendingTasks += numTasks;
	}
}

void TaskScheduler::WorkerThreadFunc(void* params)
{
	assert(params && "WorkerThreadFunc being initialized with null params");
	ThreadContext& threadContext = *(static_cast<ThreadContext*>(params));

	m_IsWorkerThread = 1;

	for (int i = 0; i < NUM_WORKERS_THREAD; ++i)
	{
		std::cout << "WorkerThreadFunc: " << threadContext.GetThreadIndex() << std::endl;
	}

	threadContext.CreateSchedulerFiber(&TaskScheduler::FiberSchedulerFunc, params);
}

void TaskScheduler::FiberSchedulerFunc(void* params)
{
	assert(params && "FiberSchedulerFunc being initialized with null params");
	ThreadContext& threadContext = *(static_cast<ThreadContext*>(params));
	TaskScheduler* taskScheduler = threadContext.GetTaskScheduler();

	std::cout << "FiberSchedulerFunc: " << threadContext.GetThreadIndex() << std::endl;

	taskScheduler->m_InitializedThreads.fetch_add(1);
	while (taskScheduler->m_InitializedThreads.load() < NUM_WORKERS_THREAD)
	{
		// Do nothing
	}

	_mm_mfence();

	while (threadContext.GetState() != ThreadContextState::EXIT)
	{
		if (ExecuteNextTask(threadContext))
		{
			std::unique_lock<std::mutex> lock(taskScheduler->m_AllTasksFinishedMutex);
			taskScheduler->m_NumPendingTasks--;
			taskScheduler->m_AllTasksFinished.notify_one();
		}
	}
}

void TaskScheduler::FiberMainFunc(void* params)
{
	assert(params && "FiberMainFunc being initialized with null params");
	FiberContext& fiberContext = *(static_cast<FiberContext*>(params));

	std::cout << "FiberMainFunc: " << std::endl;
	
	while (true)
	{
		assert(fiberContext.taskDescription.taskEntryPoint && "Invalid taskEntryPoint");
		assert(fiberContext.taskDescription.userData && "Invalid userData");

		fiberContext.state = FiberContextState::RUNNING;
		fiberContext.taskDescription.taskEntryPoint(fiberContext.taskDescription.userData, fiberContext);
		fiberContext.state = FiberContextState::FINISHED;

		fiberContext.threadContext->SwitchToSchedulerFiber();
	}
}

bool TaskScheduler::ExecuteNextTask(ThreadContext& context)
{
	TaskScheduler* taskScheduler = context.GetTaskScheduler();
	assert(taskScheduler && "TaskScheduler invalid.");

	FiberContext* fiberContext = nullptr;
	TaskDescription taskDescription;

	if (taskScheduler->m_WakeupListFiberContexts.size() > 0)
	{
		std::lock_guard<std::mutex> guard(taskScheduler->m_WakeupListFibersMutext);
		if (taskScheduler->m_WakeupListFiberContexts.size() > 0)
		{
			fiberContext = taskScheduler->m_WakeupListFiberContexts.back();
			taskScheduler->m_WakeupListFiberContexts.pop_back();
			
			// Wakeup fibers already have a task bound to them.
			taskDescription = fiberContext->taskDescription;
		}
	}
	
	if (fiberContext == nullptr)
	{
		if (taskScheduler->m_TaskDescriptionQueue.size() == 0
			|| taskScheduler->m_AvailableFiberContexts.size() == 0)
		{
			return false;
		}

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
			taskDescription = taskScheduler->m_TaskDescriptionQueue.front();
			taskScheduler->m_TaskDescriptionQueue.pop();
		}
	}
	
	fiberContext->threadContext = &context;
	fiberContext->taskDescription = taskDescription;
	fiberContext->fiber.SwitchToFiber();

	if (fiberContext->state == FiberContextState::FINISHED)
	{
		if (taskDescription.counter)
		{
			taskDescription.counter->Decrement();
		}

		// Release fiber after finishing executing the task
		std::lock_guard<std::mutex> availableFibersLock(taskScheduler->m_AvailableFibersMutext);
		fiberContext->state = FiberContextState::IDLE;
		taskScheduler->m_AvailableFiberContexts.push_back(fiberContext);
	}

	return true;
}

bool TaskScheduler::IsWorkerThread()
{
	return m_IsWorkerThread != 0;
}

void TaskScheduler::MoveFiberToWaitList(FiberContext & fiberContext)
{
	std::lock_guard<std::mutex> guard(m_WaitListFibersMutext);
	m_WaitListFiberContexts.push_back(&fiberContext);
}

void TaskScheduler::MoveFiberToWakeupList(FiberContext& fiberContext)
{
	std::lock_guard<std::mutex> guard(m_WaitListFibersMutext);
	m_WakeupListFiberContexts.push_back(&fiberContext);
}
