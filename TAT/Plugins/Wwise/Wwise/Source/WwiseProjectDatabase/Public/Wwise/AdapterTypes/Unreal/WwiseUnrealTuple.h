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
#include "Wwise/AdapterTypes/WwiseTuple.h"

template <typename... Types>
class WwiseUnrealTuple : public IWwiseTuple<TTuple<Types...>>
{
public:
	WwiseUnrealTuple()
	{
		this->Tuple = TTuple<Types...>();
	}

	WwiseUnrealTuple(const Types&... InTypes)
	{
		this->Tuple = TTuple<Types...>(InTypes...);
	}

	WwiseUnrealTuple(const TTuple<Types...>& InTuple)
	{
		this->Tuple = InTuple;
	}

	template <int N>
	auto Get() const
	{
		return this->Tuple.template Get<N>();
	}

	friend bool operator==(const WwiseUnrealTuple& Lhs, const WwiseUnrealTuple& Rhs)
	{
		return Lhs.Tuple == Rhs.Tuple;
	}
	
};

template <typename... Args>
uint32 GetTypeHash(const WwiseUnrealTuple<Args...>& InTuple)
{
	return GetTypeHash(InTuple.Tuple);
}
