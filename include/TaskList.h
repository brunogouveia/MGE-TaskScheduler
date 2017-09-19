
#pragma once

#include "TaskCounter.h"

#include <atomic>

namespace MGE
{
	typedef void(*TaskEntryPoint)(void* params, class FiberContext& context);

	enum class TaskExecutionResult
	{
		Finished,
		FinishedLast,
		Failed
	};

	class TaskList
	{
	public:
		TaskList();

		template <typename TaskType>
		TaskList(TaskType* tasks, uint32_t numTasks, std::shared_ptr<TaskCounter>& taskCounter)
			: taskEntryPoint(&TaskType::TaskEntryPoint)
			, m_Tasks(tasks)
			, m_TaskGetter(&TaskList::GetTaskFunctor<TaskType>)
			, m_Counter(taskCounter)
			, m_NumTasks(numTasks)
			, m_Valid(true)
		{

		}

		//////////////////////////////////////////////////////////////////////////

		virtual TaskExecutionResult ExecuteNextTask(class FiberContext& fiberContext);

		bool HasPendingTask() const;

		//////////////////////////////////////////////////////////////////////////

		void Invalidate();

		bool IsValid() const;

		size_t GetNumTasks() const;

		size_t GetIndex() const;
		void SetIndex(size_t index);

		TaskCounter* GetCounter() const;

		//////////////////////////////////////////////////////////////////////////

		TaskList& operator=(const TaskList& other);

	private:
		//////////////////////////////////////////////////////////////////////////

		typedef void* (*TaskGetter)(TaskList*, uint32_t);

		template <typename TaskType>
		static void* GetTaskFunctor(TaskList* taskList, uint32_t index)
		{
			assert(index < taskList->m_NumTasks && "Index out of bounds");

			return static_cast<TaskType*>(taskList->m_Tasks) + index;
		}

		//////////////////////////////////////////////////////////////////////////

		TaskEntryPoint taskEntryPoint = nullptr;
		void* m_Tasks = nullptr;
		TaskGetter m_TaskGetter = nullptr;

		size_t m_Index = 0;
		std::shared_ptr<TaskCounter> m_Counter = nullptr;

		std::atomic<uint32_t> m_CurrentTaskIndex = 0;
		uint32_t m_NumTasks = 0;

		std::atomic<bool> m_Valid = false;
	};
}