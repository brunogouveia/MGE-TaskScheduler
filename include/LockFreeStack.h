#pragma once

#include <atomic>
#include <mutex>
#include <queue>

#include <assert.h>
#include <cmath>
#include <cinttypes>

namespace MGE
{
	template<typename T, uint32_t capacity>
	class LockFreeStack
	{
	public:
	//	inline bool TryPush(const T& item)
	//	{
	//		auto oldtop = _top.load(std::memory_order_relaxed);
	//		auto oldbottom = _bottom.load(std::memory_order_relaxed);
	//		auto numtasks = oldbottom - oldtop;

	//		if (oldbottom > oldtop && // size_t is unsigned, validate the result is positive
	//			numtasks >= capacity - 1)
	//		{
	//			// The caller can decide what to do, they will probably spinwait.
	//			return false;
	//		}

	//		_values[oldbottom % capacity].store(item, std::memory_order_relaxed);
	//		_bottom.fetch_add(1, std::memory_order_release);
	//		return true;
	//	}

	//	inline bool TryPop(T& result) {

	//		size_t oldtop, oldbottom, newtop, newbottom, ot;

	//		oldbottom = _bottom.fetch_sub(1, std::memory_order_release);
	//		ot = oldtop = _top.load(std::memory_order_acquire);
	//		newtop = oldtop + 1;
	//		newbottom = oldbottom - 1;

	//		// Bottom has wrapped around.
	//		if (oldbottom < oldtop) {
	//			_bottom.store(oldtop, std::memory_order_relaxed);
	//			return false;
	//		}

	//		// The queue is empty.
	//		if (oldbottom == oldtop) {
	//			_bottom.fetch_add(1, std::memory_order_release);
	//			return false;
	//		}

	//		// Make sure that we are not contending for the item.
	//		if (newbottom == oldtop) {
	//			auto ret = _values[newbottom % capacity].load(std::memory_order_relaxed);
	//			if (!_top.compare_exchange_strong(oldtop, newtop, std::memory_order_acquire)) {
	//				_bottom.fetch_add(1, std::memory_order_release);
	//				return false;
	//			}
	//			else {
	//				result = ret;
	//				_bottom.store(newtop, std::memory_order_release);
	//				return true;
	//			}
	//		}

	//		// It's uncontended.
	//		result = _values[newbottom % capacity].load(std::memory_order_acquire);
	//		return true;
	//	}

	//	// Multiple consumer.
	//	inline bool Steal(T& result) {
	//		size_t oldtop, newtop, oldbottom;

	//		oldtop = _top.load(std::memory_order_acquire);
	//		oldbottom = _bottom.load(std::memory_order_relaxed);
	//		newtop = oldtop + 1;

	//		if (oldbottom <= oldtop)
	//			return false;

	//		// Make sure that we are not contending for the item.
	//		if (!_top.compare_exchange_strong(oldtop, newtop, std::memory_order_acquire)) {
	//			return false;
	//		}

	//		result = _values[oldtop % capacity].load(std::memory_order_relaxed);
	//		return true;
	//	}

	//	size_t size()
	//	{
	//		return _top - _bottom;
	//	}

	//private:

	//	// Circular array
	//	std::atomic<T> _values[capacity];
	//	std::atomic<size_t> _top; // queue
	//	std::atomic<size_t> _bottom; // stack
	private:
		std::mutex m_Mutex;
		T m_Data[capacity];
		std::atomic<uint32_t> m_Size = 0;

		std::atomic_flag m_Lock = ATOMIC_FLAG_INIT;

	public:

		LockFreeStack(const LockFreeStack&) = delete;

		LockFreeStack()
		{

		}

		bool TryPush(T data)
		{
			//std::lock_guard<std::mutex> lock(m_Mutex);
			while (m_Lock.test_and_set(std::memory_order_acquire))
			{}

			assert(m_Size >= 0 && m_Size < capacity);
			m_Data[m_Size++] = data;

			assert(m_Size > 0 && m_Size <= capacity);
			if (!(m_Size > 0 && m_Size <= capacity))
			{
				int i = 0;
			}

			m_Lock.clear(std::memory_order_release);

			return true;

			//uint32_t nextPos = m_Size.fetch_add(1);
			////if (!(m_Data[nextPos].load() == nullptr))
			////{
			////	int i = 0;
			////}
			////assert((m_Data[nextPos].load() == nullptr) && "Pos should have nullptr value");

			//T temp = nullptr;
			//assert(temp == nullptr && "Temp not null");
			//_mm_mfence();
			//if (!m_Data[nextPos].compare_exchange_weak(temp, data))
			//{
			//	assert("Error");
			//}

			//return true;
		}


		bool TryPop(T& data)
		{
			//std::lock_guard<std::mutex> lock(m_Mutex);
			bool result = true;
			while (m_Lock.test_and_set(std::memory_order_acquire))
			{
			}

			assert(m_Size >= 0 && m_Size <= capacity);
			if (m_Size > 0)
			{
				data = m_Data[--m_Size];
			}
			else
			{
				result = false;
			}

			assert(m_Size >= 0 && m_Size < capacity);
			if (!(m_Size >= 0 && m_Size < capacity))
			{
				int i = 0;
			}

			m_Lock.clear(std::memory_order_release);

			return result;
			//uint32_t pos;
			//do
			//{
			//	pos = m_Size.load() - 1;
			//	data = m_Data[pos];
			//} while (data == nullptr || !m_Data[pos].compare_exchange_weak(data, nullptr));

			//m_Size.fetch_sub(1);

			//return true;
		}

		uint32_t size() const
		{
			return m_Size;
		}

	};
}