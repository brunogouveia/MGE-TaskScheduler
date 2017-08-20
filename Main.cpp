// Fiber.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "ThreadContext.h"
#include "TaskScheduler.h"

#include <windows.h>
#include <iostream>
#include <functional>
#include <memory>

class Coroutine
{
public:
	using Yield = std::function<void()>;
	using Run = std::function<void(Yield)>;

	virtual ~Coroutine() = default;
	virtual void setup(Run f) = 0;
	virtual bool step() = 0;
};

class SubTaskExample
{
public:
	MGE_DECLARE_TASK(SubTaskExample)

	void Do(FiberContext& context)
	{
		puts("SubTaskExample");
	}
};

class TaskExample
{
public:
	MGE_DECLARE_TASK(TaskExample)

	void Do(FiberContext& context)
	{
		std::cout << "Test" << std::endl;
		SubTaskExample subTasks[10];
		TaskCounter* taskCounter;

		context.RunTask(subTasks, 10, &taskCounter);
		context.WaitForCounterAndFree(*taskCounter, 0);

		std::cout << "Test - done" << std::endl;
	}
};

class FiberCoroutine
	: public Coroutine
{
public:
	FiberCoroutine()
		: mCurrent(nullptr), mRunning(false)
	{
	}

	~FiberCoroutine()
	{
		if (mCurrent)
			DeleteFiber(mCurrent);
	}

	void setup2()
	{
		if (!mCurrent)
		{
			mCurrent = CreateFiber(0,
				&FiberCoroutine::proc, this);
		}
	}

	void setup(Run f) override
	{
		if (!mMain)
		{
			mMain = ConvertThreadToFiber(NULL);
		}
		mRunning = true;
		mFunction = std::move(f);

		//if (!mCurrent)
		//{
		//	mCurrent = CreateFiber(0,
		//		&FiberCoroutine::proc, this);
		//}
	}

	bool step() override
	{
		SwitchToFiber(mCurrent);
		return mRunning;
	}

	void yield()
	{
		SwitchToFiber(mMain);
	}

private:
	void run()
	{
		while (true)
		{
			mFunction([this]
			{ yield(); });
			mRunning = false;
			yield();
		}
	}

	static VOID WINAPI proc(LPVOID data)
	{
		reinterpret_cast<FiberCoroutine*>(data)->run();
	}

	static LPVOID mMain;
	LPVOID mCurrent;
	bool mRunning;
	Run mFunction;
};

LPVOID FiberCoroutine::mMain = nullptr;

void threadFoo(void* param)
{
	std::cout << "Thread foo" << std::endl;
}

int main()
{
	ThreadContext threadContext;
	threadContext.Start(&threadFoo);

	TaskScheduler taskScheduler;

	TaskExample tasks[1];
	TaskCounter* counter = nullptr;
	taskScheduler.RunTask(tasks, 1, &counter);

	//taskScheduler.WaitAllTasks();
	taskScheduler.WaitForCounterAndFree(*counter, 0);

	system("pause");

    return 0;
}

