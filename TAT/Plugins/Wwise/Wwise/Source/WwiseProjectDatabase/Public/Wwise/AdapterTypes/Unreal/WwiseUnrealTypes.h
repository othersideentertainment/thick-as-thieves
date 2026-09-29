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

#if !defined(WWISE_DB_UNREAL_TYPES) || defined(WWISE_DB_STANDARD_TYPES)
#error When using Unreal types, define only WWISE_DB_UNREAL_TYPES (not WWISE_DB_STANDARD_TYPES)
#endif

#include "Wwise/AdapterTypes/Unreal/WwiseUnrealArray.h"
#include "Wwise/AdapterTypes/Unreal/WwiseUnrealGuid.h"
#include "Wwise/AdapterTypes/Unreal/WwiseUnrealJsonObject.h"
#include "Wwise/AdapterTypes/Unreal/WwiseUnrealMap.h"
#include "Wwise/AdapterTypes/Unreal/WwiseUnrealMultiMap.h"
#include "Wwise/AdapterTypes/Unreal/WwiseUnrealPair.h"
#include "Wwise/AdapterTypes/Unreal/WwiseUnrealSet.h"
#include "Wwise/AdapterTypes/Unreal/WwiseUnrealSharedPtr.h"
#include "Wwise/AdapterTypes/Unreal/WwiseUnrealTask.h"
#include "Wwise/AdapterTypes/Unreal/WwiseUnrealAsync.h"
#include "Wwise/AdapterTypes/Unreal/WwiseUnrealParallelFor.h"
#include "Wwise/AdapterTypes/Unreal/WwiseUnrealFuture.h"
#include "Wwise/AdapterTypes/Unreal/WwiseProjectDatabaseTestingUtils.h"

template<typename Type>
using WwiseDBArray = WwiseUnrealArray<Type>;
	
using WwiseDBGuid = WwiseUnrealGuid;
	
using WwiseDBJsonObject = WwiseUnrealJsonObject;
	
template<typename Key, typename Value>
using WwiseDBMap = WwiseUnrealMap<Key, Value>;
	
template<typename Key, typename Value>
using WwiseDBMultiMap = WwiseUnrealMultiMap<Key, Value>;
	
template <typename  FirstType, typename SecondType>
using WwiseDBPair = WwiseUnrealPair<FirstType, SecondType>;
	
template <typename ObjectType>
using WwiseDBSet = WwiseUnrealSet<ObjectType>;
	
using WwiseDBString = WwiseUnrealString;

template <typename... Args>
using WwiseDBTuple = WwiseUnrealTuple<Args...>;

template<typename Type>
using WwiseDBSharedPtr = WwiseUnrealSharedPtr<Type>;

template<typename Type>
using WwiseDBTask = WwiseUnrealTask<Type>;

template<typename Type>
using WwiseDBFuture = WwiseUnrealFuture<Type>;

template<typename Type>
using WwiseDBSharedFuture = WwiseUnrealSharedFuture<Type>;

namespace WwiseDBAsync = WwiseUnrealAsync;

using namespace WwiseDBAsync;