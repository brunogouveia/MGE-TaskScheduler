#pragma once

#include "TaskCounter.h"
#include "TaskList.h"

#include <assert.h>
#include <atomic>
#include <mutex>

namespace MGE
{
	template <size_t capacity>
	class TaskManager;

	template <size_t capacity>
	class TaskProvider
	{
	public:
		TaskProvider(TaskManager<capacity>* owner)
			: m_Owner(owner)
		{

		}

		bool HasAvailableTaskList();
		TaskList* GetNextTaskList();

	private:
		TaskManager<capacity>* m_Owner = nullptr;
		size_t m_CurrentIndex = 0;
	};

	template <size_t capacity>
	class TaskManager
	{
	public:
		bool HasTaskLists() const
		{
			startIndex != stopIndex;
		}

		TaskProvider<capacity> CreateTaskProvider()
		{
			return TaskProvider<capacity>(this);
		}

		TaskList* GetFirstTaskList()
		{
			return GetFirstTaskList(startIndex.load());
		}

		TaskList* GetFirstTaskList(size_t& index)
		{
			while (index != stopIndex.load(std::memory_order_relaxed))
			{
				// Increment reference counter
				flags[index].fetch_add(1);

				TaskList* result = &data[index];
				if (result->HasPendingTask())
				{
					return result;
				}
				else
				{
					// Immediately release task list
					flags[index].fetch_sub(1);
				}

				// Increment index
				index++;
				if (index >= capacity)
				{
					index = 0;
				}
			}

			return nullptr;
		}

		void ReleaseTaskList(TaskList* taskList)
		{
			size_t index = taskList->GetIndex();

			int newFlag = flags[index].fetch_sub(1) - 1;
			if (newFlag == 0)
			{
				ReleaseTail();
			}

			assert(newFlag >= 0 && "Flag has became negative!");
		}

		bool Submit(TaskList& taskList)
		{
			std::lock_guard<std::mutex> guard(m_Mutex);

			if (stopIndex == (startIndex - 1) ||
				(startIndex == 0 && stopIndex == (capacity - 1)))
			{
				return false;
			}

			size_t nextIndex = stopIndex.load();
			assert(!data[nextIndex].HasPendingTask() && "Overriding a task list");

			taskList.SetIndex(nextIndex);
			data[nextIndex] = taskList;

			// Increment stopIndex
			if (nextIndex == (capacity - 1))
			{
				stopIndex = 0;
			}
			else
			{
				stopIndex.fetch_add(1);
			}

			return true;
		}

		bool ReleaseTail()
		{
			//std::lock_guard<std::mutex> guard(m_Mutex);
			if (startIndex == stopIndex)
			{
				return false;
			}

			size_t indexToRelease = startIndex;
			while (indexToRelease != stopIndex && flags[indexToRelease] == 0)
			{
				if (startIndex == (capacity - 1))
				{
					startIndex = 0;
				}
				else
				{
					startIndex.fetch_add(1);
				}

				data[indexToRelease].Invalidate();

				indexToRelease++;
				if (indexToRelease >= capacity)
				{
					indexToRelease = 0;
				}
			}

			return true;
		}

		size_t GetSize() const
		{
			size_t start = startIndex.load(std::memory_order_relaxed);
			size_t stop = stopIndex.load(std::memory_order_relaxed);
			size_t diff = (stop >= start) ? stop - start : (capacity - start) + stop;
			return diff;
		}

	private:
		friend class TaskProvider<capacity>;
		TaskList data[capacity];
		std::atomic<int> flags[capacity] = { 0 };
		std::atomic<size_t> startIndex = 0;
		std::atomic<size_t> stopIndex = 0;

		std::mutex m_Mutex;
	};

	template<size_t capacity>
	inline bool TaskProvider<capacity>::HasAvailableTaskList()
	{
		return m_CurrentIndex != m_Owner->stopIndex;
	}

	template <size_t capacity>
	inline TaskList* TaskProvider<capacity>::GetNextTaskList()
	{
		return m_Owner->GetFirstTaskList(m_CurrentIndex);
	}
}