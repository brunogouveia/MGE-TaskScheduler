#pragma once

#include <atomic>
#include <mutex>
#include <queue>

#define FREE 1

namespace MGE
{
	template<typename T, uint32_t CAPACITY>
	class LockFreeQueue
	{
	public:
#if FREE == 1
		bool TryPush(const T& value)
		{
			BROFILER_CATEGORY("Queue::TryPush", Profiler::Color::MistyRose);
			//std::lock_guard<std::mutex> lock(m_Mutex);
			while (m_Lock.test_and_set(std::memory_order_acquire))
			{

			}

			m_Queue.push(value);
			m_Lock.clear(std::memory_order_release);
			return true;
		}

		bool TryPop(T& value)
		{
			BROFILER_CATEGORY("Queue::TryPop", Profiler::Color::MistyRose);

			//std::lock_guard<std::mutex> lock(m_Mutex);
			bool result = true;

			while (m_Lock.test_and_set(std::memory_order_acquire))
			{
			}

			if (m_Queue.empty())
			{
				result = false;
			}
			else
			{
				value = m_Queue.front();
				m_Queue.pop();
			}

			m_Lock.clear(std::memory_order_release);

			return result;
		}

		size_t size()
		{
			//std::lock_guard<std::mutex> lock(m_Mutex);
			return m_Queue.size();
		}

		bool empty()
		{
			//std::lock_guard<std::mutex> lock(m_Mutex);
			return m_Queue.empty();
		}

	private:
		std::queue<T> m_Queue;
		std::atomic_flag m_Lock = ATOMIC_FLAG_INIT;
		std::mutex m_Mutex;

#else
		static const uint32_t ALIGNMENT = 16;
		static const uint32_t ALIGNMENT_MASK = (ALIGNMENT - 1);
		static const uint32_t MASK = (CAPACITY - 1);

		struct Cell
		{
			std::atomic<uint32_t> sequence;
			T data;
		};

		// Raw memory buffer
		byte rawMemory[sizeof(Cell) * CAPACITY + ALIGNMENT];

		// Prevent false sharing between threads
		uint8_t cacheline0[64];

		Cell* const buffer;

		// Prevent false sharing between threads
		uint8_t cacheline1[64];

		std::atomic<uint32_t> enqueuePos;

		// Prevent false sharing between threads
		uint8_t cacheline2[64];

		std::atomic<uint32_t> dequeuePos;

		inline void MoveCtor(T* element, T && val)
		{
			new(element) T(std::move(val));
		}

		inline void MoveCtor(T* element, const T& val)
		{
			new(element) T(std::move(val));
		}


	public:

		LockFreeQueue(const LockFreeQueue&) = delete;

		LockFreeQueue()
			: buffer((Cell*)(((uintptr_t)&rawMemory[0] + ALIGNMENT_MASK) & ~(uintptr_t)ALIGNMENT_MASK))
		{
			for (uint32_t i = 0; i < CAPACITY; i++)
			{
				buffer[i].sequence.store(i);
			}

			enqueuePos.store(0);
			dequeuePos.store(0);
		}

		bool TryPush(const T& data)
		{
			Cell* cell = nullptr;

			uint32_t pos = enqueuePos.load();
			for (;;)
			{
				cell = &buffer[pos & MASK];

				uint32_t seq = cell->sequence.load();
				int32_t dif = (int32_t)seq - (int32_t)pos;

				if (dif == 0)
				{
					uint32_t nowPos = enqueuePos.fetch_add(1);
					if (nowPos == pos)
					{
						break;
					}
					else
					{
						pos = nowPos;
					}
				}
				else
				{
					if (dif < 0)
					{
						return false;
					}
					else
					{
						pos = enqueuePos.load();
					}
				}
			}

			// successfully found a cell
			MoveCtor(&cell->data, std::move(data));
			cell->sequence.store(pos + 1);
			return true;
		}


		bool TryPop(T& data)
		{
			Cell* cell = nullptr;
			uint32_t pos = dequeuePos.load();

			for (;;)
			{
				cell = &buffer[pos & MASK];

				uint32_t seq = cell->sequence.load();
				int32_t dif = (int32_t)seq - (int32_t)(pos + 1);

				if (dif == 0)
				{
					uint32_t nowPos = dequeuePos.fetch_add(1);
					if (nowPos == pos)
					{
						break;
					}
					else
					{
						pos = nowPos;
					}
				}
				else
				{
					if (dif < 0)
					{
						return false;
					}
					else
					{
						pos = dequeuePos.load();
					}
				}
			}

			// successfully found a cell
			MoveCtor(&data, std::move(cell->data));
			cell->sequence.store(pos + MASK + 1);
			return true;
		}

		uint32_t size() const
		{
			return ::fabs(enqueuePos - dequeuePos);
		}

#endif
	};
}