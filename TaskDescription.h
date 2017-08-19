#pragma once

class FiberContext;

typedef void(*TaskEntryPoint)(void* params, FiberContext& context);

class TaskDescription
{
public:
	TaskEntryPoint taskEntryPoint;
	void* userData;
};