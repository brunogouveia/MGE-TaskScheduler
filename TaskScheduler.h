#pragma once

#include "TaskDescription.h"
#include "ThreadContext.h"
#include "FiberContext.h"

#include <array>
#include <atomic>
#include <mutex>
#include <queue>
#include <vector>

class TaskCounter
{
public:
	explicit TaskCounter(uint32_t initialCount)
		: m_Counter(initialCount)
	{}

	void Decrement()
	{
		m_Counter.fetch_sub(1u);

		if (m_Counter == 0 && onCompleteAction)
		{
			onCompleteAction();
		}
	}

public:
	FiberContext* fiberContext = nullptr;
	std::function<void()> onCompleteAction = nullptr;
private:
	std::atomic<uint32_t> m_Counter;
};

#define MGE_DECLARE_TASK(TaskType)									\
	static void TaskEntryPoint(void* params, FiberContext& context) \
	{																\
		static_cast<TaskType*>(params)->Do(context);				\
	}

class TaskScheduler
{
public:
	static const int NUM_STANDARD_FIBERS = 128;
	static const int NUM_WORKERS_THREAD = 2;

	explicit TaskScheduler();
	~TaskScheduler();

	template <typename Task>
	void RunTask(Task* tasks, uint32_t numTasks, TaskCounter** outTaskCounter = nullptr)
	{
		TaskCounter* taskCounter = nullptr;
		if (outTaskCounter)
		{
			taskCounter = (*outTaskCounter) = new TaskCounter(numTasks);
		}

		void* test = alloca(sizeof(TaskDescription) * numTasks);
		TaskDescription* taskDescriptions = static_cast<TaskDescription*>(test);

		// Create task descriptions
		for (size_t i = 0; i < numTasks; i++)
		{
			taskDescriptions[i] = TaskDescription{ &Task::TaskEntryPoint, tasks, taskCounter };
		}

		RunTaskImpl(taskDescriptions, numTasks, taskCounter);
	}

	void WaitForCounterAndFree(FiberContext& fiberContext, TaskCounter* counter, uint32_t value);
	void WaitForCounterAndFree(TaskCounter* counter, uint32_t value);
	void WaitAllTasks();

//private:
	void RunTaskImpl(TaskDescription* tasks, uint32_t numTasks, TaskCounter* taskCounter);

	static void WorkerThreadFunc(void* params);
	static void FiberSchedulerFunc(void* params);
	static void FiberMainFunc(void* params);

	static bool ExecuteNextTask(ThreadContext& context);

	static bool IsWorkerThread();

	void MoveFiberToWaitList(FiberContext& fiberContext);
	void MoveFiberToWakeupList(FiberContext& fiberContext);

	ThreadContext m_WorkerThreadContexts[NUM_WORKERS_THREAD];

	FiberContext m_StandardFiberContexts[NUM_STANDARD_FIBERS];

	std::vector<FiberContext*> m_AvailableFiberContexts;
	std::vector<FiberContext*> m_WaitListFiberContexts;
	std::vector<FiberContext*> m_WakeupListFiberContexts;

	std::queue<TaskDescription> m_TaskDescriptionQueue;

	std::mutex m_AvailableFibersMutext;
	std::mutex m_WaitListFibersMutext;
	std::mutex m_WakeupListFibersMutext;
	std::mutex m_TaskQueueMutext;
	std::atomic<int> m_InitializedThreads = 0;

	std::atomic<uint32_t> m_NumPendingTasks = 0;
	std::condition_variable m_AllTasksFinished;
	std::mutex m_AllTasksFinishedMutex;
};

