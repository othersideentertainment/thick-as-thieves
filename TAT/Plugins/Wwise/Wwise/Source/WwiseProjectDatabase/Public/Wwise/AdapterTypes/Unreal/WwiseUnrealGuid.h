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

#include "Wwise/AdapterTypes/WwiseDataTypesAdapter.h"
#include "Wwise/AdapterTypes/Unreal/WwiseUnrealString.h"
#include "Misc/Guid.h"
#include "Wwise/AdapterTypes/WwiseGuid.h"

class WwiseUnrealGuid : public IWwiseGuid<FGuid>
{
public:
	WwiseUnrealGuid(){}
	
	WwiseUnrealGuid(int32 A, int32 B, int32 C, int32 D)
	{
		Guid = FGuid(A, B, C, D);
	}

	WwiseUnrealGuid(const FGuid& Guid): IWwiseGuid(Guid){}

	WwiseUnrealGuid(const WwiseUnrealString& InString)
	{
		Parse(InString);
	}
	
	
	bool Parse(const WwiseUnrealString& String)
	{
		return FGuid::Parse(String.String, this->Guid);
	}

	static bool Parse(const WwiseUnrealString& String, WwiseUnrealGuid& OutGuid)
	{
		return OutGuid.Parse(String);
	}

	bool IsValid() const override
	{
		return this->Guid.IsValid();	
	}

	WwiseUnrealString ToString() const
	{
		FString String = this->Guid.ToString();
		String = String.Mid(0, 8) + "-" + String.Mid(8, 4) + "-" + String.Mid(12, 4)
		+ "-" + String.Mid(16, 4) + "-" + String.Mid(20, 12);
		return "{" + String + "}";
	}

	void GetGuidValues(int& A, int& B, int& C, int& D) const
	{
		A = this->Guid.A;
		B = this->Guid.B;
		C = this->Guid.C;
		D = this->Guid.D;
	}
};

inline uint32 GetTypeHash(const WwiseUnrealGuid& Type)
{
	return GetTypeHash(Type.Guid);
}
