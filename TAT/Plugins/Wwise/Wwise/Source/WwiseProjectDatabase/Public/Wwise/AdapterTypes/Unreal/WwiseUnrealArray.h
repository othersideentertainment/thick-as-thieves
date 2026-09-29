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

#include "Wwise/AdapterTypes/WwiseArray.h"

#include "Containers/Array.h"

template <typename ObjectType, typename UnrealArray = TArray<ObjectType>>
class WwiseUnrealArray : public IWwiseArray<UnrealArray>
{
public:
	WwiseUnrealArray(){}
	
	WwiseUnrealArray(const UnrealArray& Array)
		: IWwiseArray<UnrealArray>(Array)
	{
	}

	unsigned int Size() const override
	{
		return this->Array.Num();
	}

	void Reserve(unsigned int NumberOfElements) override
	{
		this->Array.Reserve(NumberOfElements);		
	}

	void Empty(unsigned int ExpectedUsage = 0) override
	{
		this->Array.Empty(ExpectedUsage);		
	}

	void Pop() override
	{
		this->Array.Pop();
	}

	unsigned int GetTypeSize() const override
	{
		return this->Array.GetTypeSize();
	}

	template <typename Predicate>
	bool ContainsByPredicate(Predicate InPredicate) const
	{
		return this->Array.ContainsByPredicate(InPredicate);
	}

	template <typename Predicate>
	ObjectType* FindByPredicate(Predicate InPredicate)
	{
		return this->Array.FindByPredicate(InPredicate);
	}

	template<typename... ArgsType>
	int Emplace(ArgsType&&... T)
	{
		return this->Array.Emplace(T...);
	}

	void Append(const WwiseUnrealArray<ObjectType>& T)
	{
		this->Array.Append(T.Array);	
	}

	void Add(const ObjectType& T)
	{
		this->Array.Add(T);
	}

	void Add(ObjectType&& T)
	{
		this->Array.Add(std::move(T));
	}

	ObjectType* GetData()
	{
		return this->Array.GetData();
	}

	const ObjectType* GetData() const
	{
		return this->Array.GetData();
	}

	auto& operator[](int Index)
	{
		return this->Array[Index];
	}

	const auto& operator[](int Index) const
	{
		return this->Array[Index];
	}

	const auto& operator()(int Index) const
	{
		return this->Array[Index];
	}
};