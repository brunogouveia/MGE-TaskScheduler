#pragma once

#include "TaskDescription.h"
#include "Fiber.h"

class ThreadContext;

class FiberContext
{
public:
	typedef void(*FiberFunc)(void* params);

	FiberContext();
	~FiberContext();

	void CreateFiber(FiberFunc fiberFunc, void* parameters);
	void CreateFromCurrentThreadAndRun(FiberFunc fiberFunc, void* params);

	Fiber fiber;
	TaskDescription taskDescription;

	ThreadContext* threadContext = nullptr;
};

