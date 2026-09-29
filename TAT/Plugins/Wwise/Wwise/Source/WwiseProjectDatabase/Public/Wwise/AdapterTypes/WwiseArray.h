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

template <typename ArrayType>
class IWwiseArray
{
public:
	IWwiseArray()
	{
		
	}
	
	IWwiseArray(const ArrayType& InArray)
	{
		Array = InArray;
	}

	virtual ~IWwiseArray() {}
	
	ArrayType Array;

	virtual unsigned int Size() const = 0;

	virtual void Reserve(unsigned int NumberOfElements) = 0;

	virtual void Empty(unsigned int ExpectedUsage = 0) = 0;

	virtual unsigned int GetTypeSize() const = 0;

	virtual bool IsValidIndex(int Index) const
	{
		return Index >= 0 && Index < (int)Size();
	}

	virtual void Pop() = 0;
	
	friend bool operator==(const IWwiseArray& X, const IWwiseArray& Y)
	{
		return X.Array == Y.Array;
	}

	auto begin() const
	{
		return Array.begin();
	}
	
	auto end() const
	{
		return Array.end();
	}

	auto begin()
	{
		return Array.begin();
	}
	
	auto end()
	{
		return Array.end();
	}
};