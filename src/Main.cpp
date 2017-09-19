#include "ThreadContext.h"
#include "TaskScheduler.h"
#include "FiberContext.h"

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

class SubTaskExample2
{
public:
	MGE_DECLARE_TASK(SubTaskExample2)

	void Do(MGE::FiberContext& context)
	{
		puts("SubTaskExample2");
	}
};

class TaskExample
{
public:
	MGE_DECLARE_TASK(TaskExample)

	void Do(MGE::FiberContext& context)
	{
		std::cout << "Test" << std::endl;
		
		SubTaskExample subTasks[40];
		SubTaskExample2 subTasks2[40];

		std::shared_ptr<MGE::TaskCounter> taskCounter;
		std::shared_ptr<MGE::TaskCounter> taskCounter2;

		context.RunTask(subTasks, 40, &taskCounter);
		context.RunTask(subTasks2, 40, &taskCounter2);

		context.WaitForCounterAndFree(taskCounter, 0);
		context.WaitForCounterAndFree(taskCounter2, 0);

		std::cout << "Test - done" << std::endl;
	}
};

int main()
{
	MGE::TaskScheduler taskScheduler;

	TaskExample tasks[1];
	taskScheduler.RunTask(tasks, 1);

	taskScheduler.WaitAllTasks();

	system("pause");

    return 0;
}

