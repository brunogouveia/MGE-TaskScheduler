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

class TaskExample
{
public:
	static void TaskEntryPoint(void* params, FiberContext& context)
	{
		static_cast<TaskExample*>(params)->Do(context);
	}

	void Do(FiberContext& context)
	{
		//std::cout << "Test" << std::endl;
		puts("Test");
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

#include <thread>
int main()
{
	//auto coroutine = std::make_shared<FiberCoroutine>();
	//coroutine->setup2();
	//std::thread ttest([&]() {
	//	coroutine->setup([](Coroutine::Yield yield)
	//	{
	//		for (int i = 0; i < 3; ++i)
	//		{
	//			std::cout << "Coroutine "
	//				<< i << std::endl;
	//			yield();
	//		}
	//	});
	//	//coroutine->setup([](Coroutine::Yield yield)
	//	//{
	//	//	const char test[10000] = {0};
	//	//	TaskScheduler::FiberMainFunc(nullptr);
	//	//});

	//	int stepCount = 0;
	//	while (coroutine->step())
	//	{
	//		std::cout << "Main "
	//			<< stepCount++ << std::endl;
	//	}
	//});

	ThreadContext threadContext;
	threadContext.Start(&threadFoo);

	TaskScheduler taskScheduler(2);

	TaskExample tasks[10];
	taskScheduler.RunTask(tasks, 10);
	//auto example = TaskExample::CreateParams(2, 10.0f);
	while (true) {}

	system("pause");

    return 0;
}

