#include "Fiber.h"

#include <assert.h>

namespace MGE
{
	Fiber::Fiber()
	{
	}


	Fiber::~Fiber()
	{
	}

	void Fiber::CreateFiber(FiberFunc fiberFunc, void* parameters)
	{
		m_FiberFunc = fiberFunc;
		m_FiberParams = parameters;

		m_Fiber = ::CreateFiber(128, &Fiber::FiberInternalFunc, this);
	}

	void Fiber::CreateFromCurrentThreadAndRun(FiberFunc fiberFunc, void* parameters)
	{
		m_FiberFunc = fiberFunc;
		m_FiberParams = parameters;

		assert(!::IsThreadAFiber() && "Thread is a fiber already");
		m_Fiber = ::ConvertThreadToFiber(NULL);

		m_FiberFunc(m_FiberParams);

		DeleteFiber();
	}

	void Fiber::DeleteFiber()
	{
		::DeleteFiber(m_Fiber);
		m_Fiber = nullptr;

		m_FiberFunc = nullptr;
		m_FiberParams = nullptr;
	}

	void Fiber::SwitchTo(Fiber& from, Fiber& to)
	{
		assert(from.m_Fiber && "Invalid source fiber");
		assert(to.m_Fiber && "Invalid target fiber");
		::SwitchToFiber(to.m_Fiber);
	}

	void WINAPI Fiber::FiberInternalFunc(void* parameters)
	{
		assert(parameters && "parameters is null");
		Fiber* fiber = static_cast<Fiber*>(parameters);

		assert(fiber->m_FiberFunc && "FiberFunc is null");
		assert(fiber->m_FiberParams && "FiberParams is null");
		fiber->m_FiberFunc(fiber->m_FiberParams);
	}
}