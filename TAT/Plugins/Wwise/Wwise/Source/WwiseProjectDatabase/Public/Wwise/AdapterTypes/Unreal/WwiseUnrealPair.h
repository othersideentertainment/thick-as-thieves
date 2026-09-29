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

#include "Wwise/AdapterTypes/WwisePair.h"

#include "Containers/Map.h"

template <typename FirstType, typename SecondType, typename UnrealPair = TPair<FirstType, SecondType>>
class WwiseUnrealPair : public IWwisePair<UnrealPair>
{
public:
	WwiseUnrealPair(): IWwisePair<UnrealPair>()
	{
		this->Pair = TPair<FirstType, SecondType>();
	}

	WwiseUnrealPair(const TPair<FirstType, SecondType>& InPair) : IWwisePair<UnrealPair>(InPair)
	{
	}

	WwiseUnrealPair(TPair<FirstType, SecondType>&& InPair) : IWwisePair<UnrealPair>(std::move(InPair))
	{
	}

	WwiseUnrealPair(TPair<FirstType, SecondType>& InPair) : IWwisePair<UnrealPair>(InPair)
	{
	}
	
	WwiseUnrealPair(const FirstType& Key, const SecondType& Value)
	{
		this->Pair = UnrealPair(Key, Value);
	}
	
	const FirstType& GetFirst() const
	{
		return this->Pair.template Get<0>();
	}

	const SecondType& GetSecond() const
	{
		return this->Pair.template Get<1>();
	}

	bool operator<(const WwiseUnrealPair<FirstType, SecondType>& Rhs) const
	{
		return this->Pair < Rhs.Pair;
	}
	
	bool operator==(const WwiseUnrealPair<FirstType, SecondType>& Rhs)
	{
		return this->Pair == Rhs.Pair;
	}

	bool operator!=(const WwiseUnrealPair<FirstType, SecondType>& Rhs)
	{
		return !(this->Pair == Rhs.Pair);
	}
	
};

template <typename  FirstType, typename SecondType>
uint32 GetTypeHash(const WwiseUnrealPair<FirstType, SecondType>& InPair)
{
	return GetTypeHash(InPair.Pair);
}

template <typename  FirstType, typename SecondType>
inline bool operator==(const WwiseUnrealPair<FirstType, SecondType>& Lhs, const WwiseUnrealPair<FirstType, SecondType>& Rhs)
{
	return Lhs.Pair == Rhs.Pair;
}