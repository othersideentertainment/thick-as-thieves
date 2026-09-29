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
#include "Wwise/AdapterTypes/WwiseMap.h"
#include "Wwise/AdapterTypes/Unreal/WwiseUnrealTuple.h"

template <typename KeyType, typename ObjectType, typename UnrealMap = TMap<KeyType, ObjectType>>
class WwiseUnrealMap : public IWwiseMap<UnrealMap>
{
public:
	WwiseUnrealMap(){}
	
	WwiseUnrealMap(const UnrealMap& Map)
		: IWwiseMap<UnrealMap>(Map)
	{
	}

	unsigned int Size() const override
	{
		return this->Map.Num();
	}

	void Empty(unsigned int ExpectedUsage = 0) override
	{
		this->Map.Empty(ExpectedUsage);		
	}

	void Append(const WwiseUnrealMap<KeyType, ObjectType>& T)
	{
		this->Map.Append(T.Map);	
	}

	void Add(const KeyType& Key, ObjectType&& Object)
	{
		if(!this->Map.Contains(Key))
		{
			this->Map.Add(Key, std::move(Object));
		}
	}

	void Add(const KeyType& Key, const ObjectType& Object)
	{
		if(!this->Map.Contains(Key))
		{
			this->Map.Add(Key, Object);	
		}
	}

	void Add(const WwiseUnrealTuple<KeyType, ObjectType>& Tuple)
	{
		if(!this->Map.Contains(Tuple.Tuple.Key))
		{
			this->Map.Add(Tuple.Tuple);	
		}
	}
	
	ObjectType* Find(KeyType Key)
	{
		return this->Map.Find(Key);	
	}

	const ObjectType* Find(KeyType Key) const
	{
		return this->Map.Find(Key);	
	}

	void GenerateValueArray(WwiseUnrealArray<ObjectType>& OutArray) const
	{
		this->Map.GenerateValueArray(OutArray.Array);
	}

	bool Contains(const KeyType& Key) const
	{
		return this->Map.Contains(Key);
	}
	
	ObjectType FindRef(const KeyType& Key) const
	{
		return this->Map.FindRef(Key);
	}

	const ObjectType& FindChecked(const KeyType& Key) const
	{
		return this->Map.FindChecked(Key);
	}

	ObjectType& FindChecked(const KeyType& Key)
	{
		return this->Map.FindChecked(Key);
	}

	void AddOrReplace(const KeyType& Key, const ObjectType& Object)
	{
		if(!this->Map.Contains(Key))
		{
			this->Map.Add(Key, Object);
		}
		this->Map[Key] = Object;
	}

	ObjectType operator[](KeyType Key) const
	{
		if(!this->Map.Contains(Key))
		{
			return {};
		}
		return this->Map[Key];
	}

	TPair<KeyType, ObjectType> operator()(int Index) const
	{
		int Count = 0;
		for(auto& Object : this->Map)
		{
			if(Count == Index)
			{
				return Object;
			}
			Count++;
		}
		return {};
	}

	static KeyType GetKey(const TPair<KeyType, ObjectType>& Pair)
	{
		return Pair.Key;
	}
};
