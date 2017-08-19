#include "stdafx.h"
#include "FiberContext.h"


FiberContext::FiberContext()
{
}


FiberContext::~FiberContext()
{
}

void FiberContext::CreateFiber(FiberFunc fiberFunc, void* parameters)
{
	fiber.CreateFiber(fiberFunc, parameters);
}

void FiberContext::CreateFromCurrentThreadAndRun(FiberFunc fiberFunc, void* params)
{
	fiber.CreateFromCurrentThreadAndRun(fiberFunc, params);
}
