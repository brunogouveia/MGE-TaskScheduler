#pragma once

#include "TaskDescription.h"
#include "ThreadContext.h"
#include "FiberContext.h"

#include <array>
#include <atomic>
#include <mutex>
#include <queue>
#include <vector>

class TaskScheduler
{
public:
	static const int NUM_STANDARD_FIBERS = 128;
	static const int NUM_WORKERS_THREAD = 2;

	explicit TaskScheduler(uint32_t numWorkerThreads);
	~TaskScheduler();

	template <typename TTask>
	void RunTask(TTask* tasks, uint32_t numTasks)
	{
		void* test = alloca(sizeof(TaskDescription) * numTasks);
		TaskDescription* taskDescriptions = static_cast<TaskDescription*>(test);

		for (size_t i = 0; i < numTasks; i++)
		{
			taskDescriptions[i] = TaskDescription{ &TTask::TaskEntryPoint, tasks };
		}

		RunTaskImpl(taskDescriptions, numTasks);
	}

//private:
	void RunTaskImpl(TaskDescription* tasks, uint32_t numTasks);

	static void WorkerThreadFunc(void* params);
	static void FiberSchedulerFunc(void* params);
	static void FiberMainFunc(void* params);

	static bool ExecuteNextTask(ThreadContext& context);

	ThreadContext m_WorkerThreadContexts[NUM_WORKERS_THREAD];

	FiberContext m_StandardFiberContexts[NUM_STANDARD_FIBERS];

	std::vector<FiberContext*> m_AvailableFiberContexts;

	std::queue<TaskDescription> m_TaskDescriptionQueue;

	std::mutex m_AvailableFibersMutext;
	std::mutex m_TaskQueueMutext;
	std::atomic<int> m_InitializedThreads = 0;
};

