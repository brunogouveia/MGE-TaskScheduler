#include "TaskScheduler.h"

#include <assert.h>
#include <iostream>

namespace MGE
{
	__declspec(thread) uint32_t m_IsWorkerThread = 0;

	TaskScheduler::TaskScheduler()
	{
		// Setup fibers
		for (size_t i = 0; i < NUM_STANDARD_FIBERS; ++i)
		{
			m_StandardFiberContexts[i].CreateFiber(&TaskScheduler::FiberMainFunc, &m_StandardFiberContexts[i]);
			m_AvailableFiberContexts.TryPush(&m_StandardFiberContexts[i]);
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
	}

	void TaskScheduler::WaitForCounterAndFree(FiberContext& fiberContext, TaskCounter* counter, uint32_t value)
	{
		assert(counter->GetOnCompleteAction() == nullptr, "Overriding counter.onCompleteAction function.");

		// Set complete action to move fiber to wakeup list.
		counter->SetOnCompleteAction([this, &fiberContext]()
		{
			MoveFiberToWakeupList(fiberContext);
		});
	}

	void TaskScheduler::WaitAllTasks()
	{
		std::unique_lock<std::mutex> lock(m_AllTasksFinishedMutex);
		while (m_NumPendingTasks > 0)
		{
			m_AllTasksFinished.wait(lock);
		}
	}

	void TaskScheduler::RunTaskImpl(TaskList& taskList)
	{
		// Add the tasks to the queue
		assert(m_TaskManager.Submit(taskList));

		// Increment the num of pending tasks
		m_NumPendingTasks.fetch_add(taskList.GetNumTasks(), std::memory_order_relaxed);
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

		threadContext.GetShedulerFiber().CreateFromCurrentThreadAndRun(&TaskScheduler::FiberSchedulerFunc, params);
	}

	void TaskScheduler::FiberSchedulerFunc(void* params)
	{
		assert(params && "FiberSchedulerFunc being initialized with null params");
		ThreadContext& threadContext = *(static_cast<ThreadContext*>(params));
		TaskScheduler* taskScheduler = threadContext.GetTaskScheduler();

		while (threadContext.GetState() != ThreadContextState::EXIT)
		{
			if (!ExecuteNextTask(threadContext))
			{
				if (taskScheduler->m_NumPendingTasks == 0)
				{
					std::lock_guard<std::mutex> guard(taskScheduler->m_AllTasksFinishedMutex);
					taskScheduler->m_AllTasksFinished.notify_one();
				}
			}
		}
	}

	void TaskScheduler::FiberMainFunc(void* params)
	{
		assert(params && "FiberMainFunc being initialized with null params");
		FiberContext& fiberContext = *(static_cast<FiberContext*>(params));

		TaskScheduler* taskScheduler = fiberContext.GetThreadContext()->GetTaskScheduler();
		auto taskProvider = taskScheduler->m_TaskManager.CreateTaskProvider();

		while (true)
		{
			uint32_t numTasksCompleted = 0;
			fiberContext.SetState(FiberContextState::RUNNING);

			if (taskProvider.HasAvailableTaskList())
			{
				TaskList* taskList = nullptr;
				while ((taskList = taskProvider.GetNextTaskList()))
				{
					TaskExecutionResult result;
					while ((result = taskList->ExecuteNextTask(fiberContext)) != TaskExecutionResult::Failed)
					{
						numTasksCompleted++;
					}

					taskScheduler->m_TaskManager.ReleaseTaskList(taskList);
				}

				// Decrement the number of tasks executed
				taskScheduler->m_NumPendingTasks.fetch_sub(numTasksCompleted, std::memory_order_relaxed);
				numTasksCompleted = 0;
			}
			else
			{
				if (taskScheduler->m_NumPendingTasks == 0)
				{
					std::lock_guard<std::mutex> guard(taskScheduler->m_AllTasksFinishedMutex);
					taskScheduler->m_AllTasksFinished.notify_one();
				}
			}

			// Switch back to scheduler fiber
			fiberContext.SetState(FiberContextState::FINISHED);
			Fiber::SwitchTo(fiberContext.GetFiber(), fiberContext.GetThreadContext()->GetShedulerFiber());
		}
	}

	bool TaskScheduler::ExecuteNextTask(ThreadContext& context)
	{
		TaskScheduler* taskScheduler = context.GetTaskScheduler();
		assert(taskScheduler && "TaskScheduler invalid.");

		FiberContext* fiberContext = nullptr;

		if (taskScheduler->m_WakeupListFiberContexts.size() > 0)
		{
			if (taskScheduler->m_WakeupListFiberContexts.TryPop(fiberContext))
			{
				assert(fiberContext);
				assert(fiberContext->GetState() == FiberContextState::WAITING);
			}
			else
			{
				fiberContext = nullptr;
			}
		}

		if (fiberContext == nullptr)
		{
			// Try to get a task
			if (taskScheduler->m_AvailableFiberContexts.size() == 0)
			{
				return false;
			}

			// Get next task
			if (!taskScheduler->m_AvailableFiberContexts.TryPop(fiberContext))
			{
				return false;
			}

			assert(fiberContext);
			assert(fiberContext->GetState() == FiberContextState::IDLE);
		}

		fiberContext->SetThreadContext(&context);
		Fiber::SwitchTo(context.GetShedulerFiber(), fiberContext->GetFiber());

		assert(fiberContext->GetState() == FiberContextState::FINISHED ||
			fiberContext->GetState() == FiberContextState::YIELD);

		if (fiberContext->GetState() == FiberContextState::FINISHED)
		{
			// Release fiber after finishing executing the tasks
			fiberContext->SetState(FiberContextState::IDLE);
			assert(taskScheduler->m_AvailableFiberContexts.TryPush(fiberContext) && "Try Push Available Fiber");
		}
		else if (fiberContext->GetState() == FiberContextState::YIELD)
		{
			fiberContext->SetState(FiberContextState::WAITING);
			fiberContext->ReadyToWakeup();
		}

		return true;
	}

	bool TaskScheduler::IsWorkerThread()
	{
		return m_IsWorkerThread != 0;
	}

	void TaskScheduler::MoveFiberToWakeupList(FiberContext& fiberContext)
	{
		while (!m_WakeupListFiberContexts.TryPush(&fiberContext))
		{
		}
	}
}