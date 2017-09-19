#pragma once

#include <assert.h>
#include <atomic>
#include <memory>
#include <functional>

namespace MGE
{
	class FiberContext;
	class TaskCounter;

	class TaskCounter
	{
	public:
		explicit TaskCounter(int32_t initialCount);

		//////////////////////////////////////////////////////////////////////////

		std::function<void()> GetOnCompleteAction() const;
		void SetOnCompleteAction(const std::function<void()>& onCompleteAction);

		//////////////////////////////////////////////////////////////////////////

		void Decrement(int32_t amount = 1);

	public:
		//////////////////////////////////////////////////////////////////////////

		FiberContext* fiberContext = nullptr;

	private:
		//////////////////////////////////////////////////////////////////////////

		std::function<void()> m_OnCompleteAction = nullptr;
		std::atomic<int32_t> m_Counter;
	};
}