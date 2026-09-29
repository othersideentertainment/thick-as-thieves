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

template <typename MultiMapType>
class IWwiseMultiMap
{
public:
	IWwiseMultiMap()
	{
		
	}
	
	IWwiseMultiMap(const MultiMapType& InMultiMap)
	{
		MultiMap = InMultiMap;
	}
	virtual ~IWwiseMultiMap() = default;
	
	MultiMapType MultiMap;

	virtual unsigned int Size() const
	{
		return 0;
	}

	virtual void Reserve(int NumberOfElements)
	{
		
	}

	virtual void Empty(int ExpectedUsage = 0)
	{
		
	}

	virtual bool IsValidIndex(int Index) const
	{
		return Index >= 0 && Index < (int)Size();
	}
	
	friend bool operator==(const IWwiseMultiMap& X, const IWwiseMultiMap& Y)
	{
		return X.MultiMap == Y.MultiMap;
	}
	
	auto& operator[](int Index) const
	{
		return MultiMap[Index];
	}

	auto begin() const
	{
		return MultiMap.begin();
	}
	
	auto end() const
	{
		return MultiMap.end();
	}

	auto begin()
	{
		return MultiMap.begin();
	}
	
	auto end()
	{
		return MultiMap.end();
	}
};