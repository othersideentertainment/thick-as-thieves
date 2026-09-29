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

#include "WwiseUnrealDefines.h"

// We use Catch2 Test Harness, available starting UE5.1
#if UE_5_2_OR_LATER
#include "Tests/TestHarnessAdapter.h"
#else
#include "Tests/TestHarness.h"
#endif // UE_5_2_OR_LATER

// Determine if we have unit tests. These Unreal macros are defined in the previously included header files.
#if !defined(WWISE_LOW_LEVEL_TESTS)
#if defined(WITH_LOW_LEVEL_TESTS) && WITH_LOW_LEVEL_TESTS
#define WWISE_LOW_LEVEL_TESTS 1
#else
#define WWISE_LOW_LEVEL_TESTS 0
#endif
#endif

#if !defined(WWISE_AUTOMATION_TESTS)
#if defined(WITH_AUTOMATION_TESTS) && WITH_AUTOMATION_TESTS
#define WWISE_AUTOMATION_TESTS 1
#else
#define WWISE_AUTOMATION_TESTS 0
#endif
#endif

#if !defined(WWISE_UNIT_TESTS)
#if WWISE_LOW_LEVEL_TESTS || WWISE_AUTOMATION_TESTS
#define WWISE_UNIT_TESTS 1
#else
#define WWISE_UNIT_TESTS 0
#endif
#endif

//
// Definition of Wwise Unit Tests API. Unreal's Test API improved throughout the earlier UE5 versions. This
// allows for a normalized interface starting from UE4.27.
//
#if WWISE_UNIT_TESTS
#include <type_traits>


#if !UE_5_3_OR_LATER && !defined(FAIL_ON_MESSAGE)
#define FAIL_ON_MESSAGE(Message) FAutomationTestFramework::Get().GetCurrentTest()->AddExpectedError(Message);
#endif

// Some of our testing actually prefer to use the fast version of HashCombine. This is only accessible in UE5.
inline uint32 WwiseHashCombineFast(uint32 A, uint32 B)
{
	return HashCombineFast(A, B);
}


// Add logging facilities when running in Automation
#if WWISE_AUTOMATION_TESTS
#define WWISE_TEST_LOG(Format, ...) FAutomationTestFramework::Get().GetCurrentTest()->AddInfo(FString::Printf(TEXT(Format), __VA_ARGS__));
#else
#define WWISE_TEST_LOG(Format, ...) (void)0
#endif


// Use a normalized test case that works correctly with multiple compilation units (Unreal's TEST_CASE doesn't like being used in more than one module) 
#if WWISE_LOW_LEVEL_TESTS
#define WWISE_TEST_CASE(ClassName, PrettyName, Flags) TEST_CASE(PrettyName, Flags)
#elif UE_5_3_OR_LATER
#define WWISE_TEST_CASE(ClassName, PrettyName, Flags) TEST_CASE_NAMED(FWwiseTest ## ClassName, PrettyName, Flags)
#else
#define WWISE_TEST_CASE(ClassName, PrettyName, Flags) TEST_CASE_NAMED(FWwiseTest ## ClassName, LLT_STR_EXPAND(FWwiseTest ## ClassName), PrettyName, Flags)
#endif

#define WWISE_TEST_ASYNC_NAME TEXT("WwiseUnitTests Async")

#endif // WWISE_UNIT_TESTS