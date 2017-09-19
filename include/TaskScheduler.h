#pragma once

#include "FiberContext.h"
#include "TaskCounter.h"
#include "TaskList.h"
#include "TaskManager.h"
#include "ThreadContext.h"

#include "LockFreeQueue.h"
#include "LockFreeStack.h"

#include <array>
#include <atomic>
#include <mutex>
#include <queue>
#include <vector>

#define MGE_DECLARE_TASK(TaskType)									\
	static void TaskEntryPoint(void* params, MGE::FiberContext& context) \
	{																\
		static_cast<TaskType*>(params)->Do(context);				\
	}

namespace MGE
{
	class TaskScheduler
	{
	public:
		//////////////////////////////////////////////////////////////////////////

		static const int NUM_STANDARD_FIBERS = 128;
		static const int NUM_WORKERS_THREAD = 2;
		static const int NUM_TASK_LISTS = 1024;

		//////////////////////////////////////////////////////////////////////////

		TaskScheduler();
		~TaskScheduler();

		//////////////////////////////////////////////////////////////////////////

		template <typename Task>
		void RunTask(Task* tasks, uint32_t numTasks, TaskCounter** outTaskCounter = nullptr)
		{
			std::shared_ptr<TaskCounter> taskCounter = nullptr;
			if (outTaskCounter)
			{
				taskCounter = std::make_shared<TaskCounter>(numTasks);
				(*outTaskCounter) = taskCounter.get();
			}

			TaskList taskList(tasks, numTasks, taskCounter);

			RunTaskImpl(taskList);
		}

		//////////////////////////////////////////////////////////////////////////

		void WaitForCounterAndFree(FiberContext& fiberContext, TaskCounter* counter, uint32_t value);
		void WaitAllTasks();

		//////////////////////////////////////////////////////////////////////////

		static TaskScheduler& GetInstance()
		{
			static TaskScheduler taskScheduler;
			return taskScheduler;
		}

	private:
		//////////////////////////////////////////////////////////////////////////

		friend class FiberContext;

		//////////////////////////////////////////////////////////////////////////

		void RunTaskImpl(TaskList& taskList);

		//////////////////////////////////////////////////////////////////////////

		static void WorkerThreadFunc(void* params);
		static void FiberSchedulerFunc(void* params);
		static void FiberMainFunc(void* params);

		//////////////////////////////////////////////////////////////////////////

		static bool ExecuteNextTask(ThreadContext& context);

		static bool IsWorkerThread();

		void MoveFiberToWakeupList(FiberContext& fiberContext);

		//////////////////////////////////////////////////////////////////////////

		ThreadContext m_WorkerThreadContexts[NUM_WORKERS_THREAD];

		FiberContext m_StandardFiberContexts[NUM_STANDARD_FIBERS];

		MGE::LockFreeStack<FiberContext*, NUM_STANDARD_FIBERS> m_AvailableFiberContexts;
		MGE::LockFreeStack<FiberContext*, NUM_STANDARD_FIBERS> m_WakeupListFiberContexts;

		TaskManager<NUM_TASK_LISTS> m_TaskManager;

		std::atomic<uint32_t> m_NumPendingTasks = 0;
		std::condition_variable m_AllTasksFinished;
		std::mutex m_AllTasksFinishedMutex;
	};
}