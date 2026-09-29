/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/unreal
 
License Usage
 
Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2025 Audiokinetic Inc.
*******************************************************************************/

#pragma once

#include "Wwise/WwiseUnrealVersion.h"
#include "Wwise/Stats/NamedEvents.h"

#include "Tasks/Task.h"
#include "Async/Async.h"

/// Adaptor on UE::Tasks::ETaskPriority ENamedThreads::Type, with additional GameThread
enum class EWwiseTaskPriority : int8
{
	High = static_cast<int8>(UE::Tasks::ETaskPriority::High),
	Normal = static_cast<int8>(UE::Tasks::ETaskPriority::Normal),
	Default = static_cast<int8>(UE::Tasks::ETaskPriority::Default),
	BackgroundHigh = static_cast<int8>(UE::Tasks::ETaskPriority::BackgroundHigh),
	BackgroundNormal = static_cast<int8>(UE::Tasks::ETaskPriority::BackgroundNormal),
	BackgroundLow = static_cast<int8>(UE::Tasks::ETaskPriority::BackgroundLow),
	GameThread = static_cast<int8>(-1)
};

/// Adaptor on UE::Tasks::ETaskFlags
#if UE_5_4_OR_LATER
using EWwiseTaskFlags = UE::Tasks::ETaskFlags;
#else
enum class EWwiseTaskFlags
{
	None,
	DoNotRunInsideBusyWait // Ignored: do not pick this task for busy-waiting
};
#endif

/// Launches a task for asynchronous execution. Equivalent to UE::Tasks::Launch.
/// @param DebugName A unique name for task identification in debugger and profiler, is compiled out in test/shipping builds
/// @param InPriority Task priority that affects when the task will be executed
/// @param InTaskFlags Task config options
/// @param InFunction A functor that will be executed asynchronously
static void LaunchWwiseTask(
	const TCHAR* DebugName,
	EWwiseTaskPriority InPriority,
	EWwiseTaskFlags InTaskFlags,
	TUniqueFunction<void()>&& InFunction)
{
	if (InPriority == EWwiseTaskPriority::GameThread)
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady(
		[DebugName, InFunction = MoveTemp(InFunction)]
		{
			SCOPED_NAMED_EVENT_TCHAR_CONDITIONAL(DebugName, WwiseNamedEvents::Color3, DebugName != nullptr);
			InFunction();
		},
		{}, nullptr, ENamedThreads::GameThread);
	}
	else
	{
		UE::Tasks::Launch(DebugName, MoveTemp(InFunction),
			static_cast<UE::Tasks::ETaskPriority>(InPriority)
#if UE_5_4_OR_LATER
			, UE::Tasks::EExtendedTaskPriority::None, InTaskFlags
#endif
		);
	}
}

/// Launches a task for asynchronous execution. Equivalent to UE::Tasks::Launch.
/// @param DebugName A unique name for task identification in debugger and profiler, is compiled out in test/shipping builds
/// @param InPriority Task priority that affects when the task will be executed
/// @param InFunction A functor that will be executed asynchronously
static void LaunchWwiseTask(
	const TCHAR* DebugName,
	EWwiseTaskPriority InPriority,
	TUniqueFunction<void()>&& InFunction)
{
	if (InPriority == EWwiseTaskPriority::GameThread)
	{
		FFunctionGraphTask::CreateAndDispatchWhenReady(
		[DebugName, InFunction = MoveTemp(InFunction)]
		{
			SCOPED_NAMED_EVENT_TCHAR_CONDITIONAL(DebugName, WwiseNamedEvents::Color3, DebugName != nullptr);
			InFunction();
		},
		{}, nullptr, ENamedThreads::GameThread);
	}
	else
	{
		UE::Tasks::Launch(DebugName, MoveTemp(InFunction), static_cast<UE::Tasks::ETaskPriority>(InPriority));
	}
}

/// Launches a task for asynchronous execution. Equivalent to UE::Tasks::Launch.
/// @param DebugName A unique name for task identification in debugger and profiler, is compiled out in test/shipping builds
/// @param InFunction A functor that will be executed asynchronously
static void LaunchWwiseTask(
	const TCHAR* DebugName,
	TUniqueFunction<void()>&& InFunction)
{
	UE::Tasks::Launch(DebugName, MoveTemp(InFunction), static_cast<UE::Tasks::ETaskPriority>(EWwiseTaskPriority::Default));
}
