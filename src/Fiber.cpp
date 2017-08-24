#include "stdafx.h"
#include "Fiber.h"

#include <assert.h>
#include <windows.h>

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

		m_Fiber = ::CreateFiber(0, &Fiber::FiberInternalFunc, this);
	}

	void Fiber::CreateFromCurrentThreadAndRun(FiberFunc fiberFunc, void* parameters)
	{
		m_FiberFunc = fiberFunc;
		m_FiberParams = parameters;

		assert(!::IsThreadAFiber(), "Thread is a fiber already");
		//if (!m_Fiber)
		//{
		m_Fiber = ::ConvertThreadToFiber(NULL);
		//}

		m_FiberFunc(m_FiberParams);

		//DeleteFiber();
	}

	void Fiber::DeleteFiber()
	{
		assert(false, "NOOOO");
		::DeleteFiber(m_Fiber);
		m_Fiber = nullptr;

		m_FiberFunc = nullptr;
		m_FiberParams = nullptr;
	}

	void Fiber::SwitchToFiber() const
	{
		assert(m_Fiber, "Invalid fiber");
		::SwitchToFiber(m_Fiber);
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