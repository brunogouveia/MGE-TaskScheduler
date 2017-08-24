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

		void CreateSchedulerFiber(Fiber::FiberFunc fiberEntryPoint, void* params);
		void SwitchToSchedulerFiber();

		uint32_t GetThreadIndex() const;
		void SetThreadIndex(uint32_t index);

		ThreadContextState GetState() const;
		void SetState(ThreadContextState state);

		TaskScheduler* GetTaskScheduler() const;
		void SetTaskScheduler(TaskScheduler* taskScheduler);
	private:
		WorkerThreadFunc m_WorkerThreadFunc = nullptr;
		std::unique_ptr<std::thread> m_Thread;
		uint32_t m_ThreadIndex;

		ThreadContextState m_State = ThreadContextState::IDLE;
		FiberContext m_SchedulerFiberContext;
		TaskScheduler* m_TaskScheduler = nullptr;
	};
}