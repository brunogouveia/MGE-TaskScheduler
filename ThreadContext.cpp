#include "stdafx.h"
#include "ThreadContext.h"


ThreadContext::ThreadContext()
{

}


ThreadContext::~ThreadContext()
{
	if (m_Thread == nullptr)
	{
		return;
	}

	if (m_Thread->joinable())
	{
		m_Thread->join();
	}
	else
	{
		m_Thread->detach();
	}
}

void ThreadContext::Start(WorkerThreadFunc workerThreadFunc)
{
	state = ThreadContextState::RUNNING;
	m_Thread.reset(new std::thread(workerThreadFunc, this));
}

void ThreadContext::Stop()
{
	m_Thread.reset();
}

uint32_t ThreadContext::GetThreadIndex() const
{
	return m_ThreadIndex;
}

void ThreadContext::SetThreadIndex(uint32_t index)
{
	m_ThreadIndex = index;
}

void ThreadContext::Run()
{
}
