// Fiber.cpp : Defines the entry point for the console application.
//
#include "ThreadContext.h"
#include "TaskScheduler.h"

#include <windows.h>
#include <iostream>
#include <functional>
#include <memory>

class SubTaskExample
{
public:
	MGE_DECLARE_TASK(SubTaskExample)

	void Do(MGE::FiberContext& context)
	{
		puts("SubTaskExample");
	}
};

class TaskExample
{
public:
	MGE_DECLARE_TASK(TaskExample)

	void Do(MGE::FiberContext& context)
	{
		std::cout << "Test" << std::endl;
		SubTaskExample subTasks[10];
		MGE::TaskCounter* taskCounter;

		context.RunTasks(subTasks, 10, &taskCounter);
		context.WaitForCounterAndFree(taskCounter, 0);

		std::cout << "Test - done" << std::endl;
	}
};

int main()
{
	MGE::TaskScheduler taskScheduler;

	TaskExample tasks[1];
	taskScheduler.RunTasks(tasks, 1);

	taskScheduler.WaitAllTasks();

	system("pause");

    return 0;
}

