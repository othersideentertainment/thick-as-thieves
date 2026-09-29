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
#include "Wwise/AdapterTypes/WwiseSet.h"
#include "Wwise/AdapterTypes/Unreal/WwiseUnrealArray.h"

#include "Containers/Set.h"

template <typename ObjectType, typename UnrealSet = TSet<ObjectType>>
class WwiseUnrealSet : public IWwiseSet<UnrealSet>
{
public:
	WwiseUnrealSet(){}
	
	WwiseUnrealSet(const UnrealSet& Set)
		: IWwiseSet<UnrealSet>(Set)
	{
	}

	WwiseUnrealSet(const WwiseUnrealArray<ObjectType>& Array)
	{
		this->Set = TSet<ObjectType>(Array.Array);
	}

	unsigned int Size() const override
	{
		return this->Set.Num();
	}

	void Reserve(int NumberOfElements) override
	{
		this->Set.Reserve(NumberOfElements);		
	}

	WwiseUnrealSet Difference(const WwiseUnrealSet& OtherSet)
	{
		return this->Set.Difference(OtherSet.Set);
	}

	void Empty(int ExpectedUsage = 0) override
	{
		this->Set.Empty(ExpectedUsage);		
	}

	void Append(const WwiseUnrealArray<ObjectType>& T)
	{
		this->Set.Append(T.Array);
	}

	void Append(const WwiseUnrealSet<ObjectType>& T)
	{
		this->Set.Append(T.Set);	
	}

	void Add(const ObjectType& T, bool* bIsAlreadyInSetPtr = nullptr)
	{
		this->Set.Add(T, bIsAlreadyInSetPtr);	
	}

	template<typename KeyType>
	const ObjectType* Find(KeyType T) const
	{
		return this->Set.Find(T);	
	}

	void Emplace(const ObjectType& T, bool* bIsAlreadyInSetPtr = nullptr)
	{
		this->Set.Emplace(T, bIsAlreadyInSetPtr);
	}

	WwiseUnrealArray<ObjectType> AsArray() const
	{
		return this->Set.Array();
	}

	bool Contains(ObjectType Type) const
	{
		return this->Set.Contains(Type);
	}

	ObjectType GetFirst() const
	{
		const typename TSet<ObjectType>::TConstIterator FirstAuxBus(this->Set);
		return *FirstAuxBus;
	}

	template<typename Array>
	void AppendTo(Array& ArrayToAppend)
	{
		auto SetArray = this->Set.Array();
		auto Data = SetArray.GetData();
		ArrayToAppend.Append(Data, this->Set.Num());
	}
};
