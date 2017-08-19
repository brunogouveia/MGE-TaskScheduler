#pragma once

#include <windows.h>

class Fiber
{
public:
	typedef void(*FiberFunc)(void*);

	Fiber();
	~Fiber();

	void CreateFiber(FiberFunc fiberFunc, void* parameters);
	void CreateFromCurrentThreadAndRun(FiberFunc fiberFunc, void* parameters);

	void DeleteFiber();

	void SwitchToFiber() const;

private:
	static void WINAPI FiberInternalFunc(void* parameters);

	void* m_Fiber = nullptr;

	FiberFunc m_FiberFunc = nullptr;
	void* m_FiberParams = nullptr;
};

