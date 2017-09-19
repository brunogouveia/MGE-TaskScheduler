#pragma once

#include "TaskList.h"
#include "Fiber.h"

#include <stdint.h>

namespace MGE
{
	class ThreadContext;
	class TaskCounter;

	enum class FiberContextState
	{
		IDLE,
		RUNNING,
		WAITING,
		YIELD,
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
		void RunTask(Task* tasks, uint32_t numTasks, std::shared_ptr<TaskCounter>* outTaskCounter = nullptr)
		{
			std::shared_ptr<TaskCounter> taskCounter = nullptr;
			if (outTaskCounter)
			{
				taskCounter = std::make_shared<TaskCounter>(numTasks);
				(*outTaskCounter) = taskCounter;
			}

			TaskList taskList(tasks, numTasks, taskCounter);

			RunTaskImpl(taskList);
		}

		void ReadyToWakeup();
		void WaitForCounterAndFree(std::shared_ptr<TaskCounter> taskCounter, uint32_t value);

		Fiber& GetFiber();

		FiberContextState GetState() const;
		void SetState(FiberContextState state);

		ThreadContext* GetThreadContext() const;
		void SetThreadContext(ThreadContext* threadContext);

	private:
		void RunTaskImpl(TaskList& taskList);

		Fiber m_Fiber;
		std::atomic<FiberContextState> m_State = FiberContextState::IDLE;

		ThreadContext* m_ThreadContext = nullptr;
		std::shared_ptr<TaskCounter> m_TaskCounter;
	};
}