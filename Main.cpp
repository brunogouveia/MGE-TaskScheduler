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

		context.RunTasks(subTasks, 10, &taskCounter);
		context.WaitForCounterAndFree(taskCounter, 0);

		std::cout << "Test - done" << std::endl;
	}
};

int main()
{
	TaskScheduler taskScheduler;

	TaskExample tasks[1];
	taskScheduler.RunTasks(tasks, 1);

	taskScheduler.WaitAllTasks();

	system("pause");

    return 0;
}

