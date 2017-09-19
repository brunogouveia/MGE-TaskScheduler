#include "TaskList.h"

namespace MGE
{
	TaskList::TaskList()
	{
	}

	TaskExecutionResult TaskList::ExecuteNextTask(class FiberContext& fiberContext)
	{
		if (m_CurrentTaskIndex >= m_NumTasks)
		{
			return TaskExecutionResult::Failed;
		}

		uint32_t nextTaskIndex = m_CurrentTaskIndex.fetch_add(1);
		if (nextTaskIndex < m_NumTasks)
		{
			taskEntryPoint(m_TaskGetter(this, nextTaskIndex), fiberContext);

			if (m_Counter)
			{
				m_Counter->Decrement();
			}

			return nextTaskIndex == (m_NumTasks - 1) ? TaskExecutionResult::FinishedLast : TaskExecutionResult::Finished;
		}
		else
		{
			// Over added, need to fix it
			m_CurrentTaskIndex.fetch_sub(1);
			return TaskExecutionResult::Failed;
		}
	}

	bool TaskList::HasPendingTask() const
	{
		return m_CurrentTaskIndex < m_NumTasks && IsValid();
	}

	void TaskList::Invalidate()
	{
		m_Valid.store(false);
	}

	bool TaskList::IsValid() const
	{
		return m_Valid;
	}

	size_t TaskList::GetNumTasks() const
	{
		return m_NumTasks;
	}

	size_t TaskList::GetIndex() const
	{
		return m_Index;
	}

	void TaskList::SetIndex(size_t index)
	{
		this->m_Index = index;
	}

	TaskCounter* TaskList::GetCounter() const
	{
		return m_Counter.get();
	}

	TaskList& TaskList::operator=(const TaskList& other)
	{
		assert(!IsValid() && "Assigning to a Valid TaskList");

		taskEntryPoint = other.taskEntryPoint;
		m_Tasks = other.m_Tasks;
		m_TaskGetter = other.m_TaskGetter;
		m_Index = other.m_Index;
		m_Counter = other.m_Counter;
		m_CurrentTaskIndex = other.m_CurrentTaskIndex.load();
		m_NumTasks = other.m_NumTasks;
		m_Valid = other.m_Valid.load();

		return *this;
	}
}