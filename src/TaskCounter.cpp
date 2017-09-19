#include "TaskCounter.h"

namespace MGE
{
	TaskCounter::TaskCounter(int32_t initialCount)
		: m_Counter(initialCount + 1)
	{
	}

	std::function<void()> TaskCounter::GetOnCompleteAction() const
	{
		return m_OnCompleteAction;
	}

	void TaskCounter::SetOnCompleteAction(const std::function<void()>& onCompleteAction)
	{
		m_OnCompleteAction = onCompleteAction;
	}

	void TaskCounter::Decrement(int32_t amount)
	{
		int counter = m_Counter.fetch_sub(amount, std::memory_order_relaxed) - amount;

		if (counter == 0 && m_OnCompleteAction)
		{
			m_OnCompleteAction();
		}
	}
}