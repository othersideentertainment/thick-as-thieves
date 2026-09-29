// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "OSESenseConfig_Damage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESenseConfig_Damage)

UOSESenseConfig_Damage::UOSESenseConfig_Damage(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) 
{
	DebugColor = FColor::Red;
}

TSubclassOf<UOSESense> UOSESenseConfig_Damage::GetSenseImplementation() const
{
	return Implementation;
}

