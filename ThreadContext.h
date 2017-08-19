#pragma once

#include "FiberContext.h"

#include <atomic>
#include <memory>
#include <thread>

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

	FiberContext schedulerFiberContext;
	ThreadContextState state = ThreadContextState::IDLE;

	TaskScheduler* taskScheduler = nullptr;
private:
	void Run();

	WorkerThreadFunc m_WorkerThreadFunc = nullptr;
	std::unique_ptr<std::thread> m_Thread;
	uint32_t m_ThreadIndex;
};

