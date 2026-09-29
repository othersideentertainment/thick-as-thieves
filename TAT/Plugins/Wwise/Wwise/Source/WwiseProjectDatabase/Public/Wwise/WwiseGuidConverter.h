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

#include "CoreMinimal.h"
#include "Wwise/AdapterTypes/WwiseWrapperTypes.h"

namespace FWwiseGuidConverter
{
#if defined(WWISE_DB_STANDARD_TYPES)
	inline FGuid ToFGuid(const WwiseDBGuid& InGuid)
	{
		int A;
		int B;
		int C;
		int D;
		InGuid.GetGuidValues(A, B, C, D);
		return FGuid(A, B, C, D);
	}
	
	inline WwiseDBGuid ToWwiseDBGuid(const FGuid& InGuid)
	{
		return WwiseDBGuid(InGuid.A, InGuid.B, InGuid.C, InGuid.D);
	}
#elif defined(WWISE_DB_UNREAL_TYPES)
	inline FGuid ToFGuid(const WwiseDBGuid& InGuid)
	{
		return InGuid.Guid;
	}
	
	inline WwiseDBGuid ToWwiseDBGuid(const FGuid& InGuid)
	{
		return WwiseDBGuid(InGuid);
	}
#endif

};
