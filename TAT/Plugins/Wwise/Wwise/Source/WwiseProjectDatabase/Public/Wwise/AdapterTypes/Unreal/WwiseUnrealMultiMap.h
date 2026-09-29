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
#include "Wwise/AdapterTypes/Unreal/WwiseUnrealArray.h"
#include "Wwise/AdapterTypes/WwiseMultiMap.h"

template <typename KeyType, typename ObjectType, typename UnrealMultiMap = TMultiMap<KeyType, ObjectType>>
class WwiseUnrealMultiMap : public IWwiseMultiMap<UnrealMultiMap>
{
public:
	WwiseUnrealMultiMap(){}
	
	WwiseUnrealMultiMap(const UnrealMultiMap& MultiMap)
		: IWwiseMultiMap<UnrealMultiMap>(MultiMap)
	{
	}

	unsigned int Size() const override
	{
		return this->MultiMap.Num();
	}

	void Empty(int ExpectedUsage = 0) override
	{
		this->MultiMap.Empty(ExpectedUsage);		
	}

	void Append(const WwiseUnrealMultiMap<KeyType, ObjectType>& T)
	{
		this->MultiMap.Append(T.MultiMap);	
	}

	void Add(const KeyType& Key, const ObjectType& Object)
	{
		this->MultiMap.Add(Key, Object);	
	}

	void MultiFind(KeyType Key, WwiseUnrealArray<ObjectType>& OutValues, bool bMaintainOrder = true) const
	{
		this->MultiMap.MultiFind(Key, OutValues.Array, bMaintainOrder);
	}
	
	void MultiFindPointer(KeyType Key, WwiseUnrealArray<const ObjectType*>& OutValues, bool bMaintainOrder = true) const
	{
		this->MultiMap.MultiFindPointer(Key, OutValues.Array, bMaintainOrder);
	}

	const ObjectType* At(int Index) const
	{
		int Count = 0;
		for(auto& Element : this->MultiMap)
		{
			if(Count == Index)
			{
				return &Element.Value;
			}
			Count++;
		}
		return nullptr;
	}

	bool Contains(const KeyType& Key) const
	{
		return this->MultiMap.Contains(Key);
	}
};

