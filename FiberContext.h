#pragma once

#include "TaskDescription.h"
#include "Fiber.h"

#include <malloc.h>
#include <stdint.h>

class ThreadContext;
class TaskCounter;

enum class FiberContextState
{
	IDLE,
	RUNNING,
	WAITING,
	FINISHED
};

class FiberContext
{
public:
	typedef void(*FiberFunc)(void* params);

	FiberContext();
	~FiberContext();

	void CreateFiber(FiberFunc fiberFunc, void* parameters);
	void CreateFromCurrentThreadAndRun(FiberFunc fiberFunc, void* params);

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

	void WaitForCounterAndFree(TaskCounter& taskCounter, uint32_t value);

	Fiber fiber;
	FiberContextState state = FiberContextState::IDLE;
	TaskDescription taskDescription;

	ThreadContext* threadContext = nullptr;

private:
	void RunTaskImpl(TaskDescription* tasks, uint32_t numTasks, TaskCounter* taskCounter);
};

