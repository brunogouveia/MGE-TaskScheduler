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
	typedef void(*FiberEntryPoint)(void* params);

	FiberContext();
	~FiberContext();

	void CreateFiber(FiberEntryPoint fiberEntryPoint, void* parameters);
	void CreateFromCurrentThreadAndRun(FiberEntryPoint fiberEntryPoint, void* params);

	template <typename Task>
	void RunTasks(Task* tasks, uint32_t numTasks, TaskCounter** outTaskCounter = nullptr)
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
			taskDescriptions[i] = TaskDescription{ &Task::TaskEntryPoint, &tasks[i], taskCounter };
		}

		RunTask(taskDescriptions, numTasks, taskCounter);
	}

	template <typename Task>
	void RunTasksAndWait(Task* tasks, uint32_t numTasks)
	{
		TaskCounter* taskCounter;
		RunTask(tasks, numTasks, &taskCounter);
		WaitForCounterAndFree(taskCounter, 0);
	}

	void RunTask(TaskDescription* tasks, uint32_t numTasks, TaskCounter* taskCounter);

	void WaitForCounterAndFree(TaskCounter* taskCounter, uint32_t value);

	ThreadContext* GetThreadContext() const;
	void SetThreadContext(ThreadContext* threadContext);

	Fiber fiber;
	TaskDescription taskDescription;
	FiberContextState state = FiberContextState::IDLE;

private:
	ThreadContext* m_ThreadContext = nullptr;
};

