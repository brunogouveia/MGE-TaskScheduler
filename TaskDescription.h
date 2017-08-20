#pragma once

class FiberContext;
class TaskCounter;

typedef void(*TaskEntryPoint)(void* params, FiberContext& context);

class TaskDescription
{
public:
	TaskEntryPoint taskEntryPoint;
	void* userData;
	TaskCounter* counter;
};