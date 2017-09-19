#pragma once

#include "FiberContext.h"

#include <atomic>
#include <memory>
#include <thread>

namespace MGE
{
	class TaskScheduler;

	enum class ThreadContextState
	{
		IDLE,
		RUNNING,
		EXIT
	};

	class ThreadContext
	{
	public:
		typedef void(*WorkerThreadFunc)(void* params);

		ThreadContext(const ThreadContext&) = delete;

		ThreadContext();
		~ThreadContext();

		void Start(WorkerThreadFunc workerThreadFunc);
		void Stop();

		uint32_t GetThreadIndex() const;
		void SetThreadIndex(uint32_t index);

		Fiber& GetShedulerFiber();

		TaskScheduler* GetTaskScheduler() const;
		void SetTaskScheduler(TaskScheduler* taskScheduler);

		ThreadContextState GetState() const;
		void SetState(ThreadContextState state);

	private:
		std::unique_ptr<std::thread> m_Thread;
		uint32_t m_ThreadIndex;

		FiberContext m_SchedulerFiberContext;
		TaskScheduler* m_TaskScheduler = nullptr;

		std::atomic<ThreadContextState> m_State = ThreadContextState::IDLE;
	};

}