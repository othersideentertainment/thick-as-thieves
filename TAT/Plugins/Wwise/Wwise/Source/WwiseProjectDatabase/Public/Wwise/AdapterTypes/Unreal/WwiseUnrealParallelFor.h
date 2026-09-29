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

#include "WwiseUnrealMap.h"
#include "Wwise/AdapterTypes/Unreal/WwiseUnrealArray.h"
#include "Async/ParallelFor.h"

namespace WwiseUnrealAsync
{
	template<typename ArrayType, typename CallableType>
	void WwiseParallelFor(const WwiseUnrealArray<ArrayType>& Array, CallableType&& Type)
	{
		ParallelFor(Array.Size(), std::move(Type), EParallelForFlags::BackgroundPriority);
	}

	template<typename KeyType, typename ValueType, typename CallableType>
	void WwiseParallelFor(const WwiseUnrealMap<KeyType, ValueType>& Map, CallableType&& Type)
	{
		ParallelFor(Map.Size(), std::move(Type), EParallelForFlags::BackgroundPriority);
	}
}