// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "OSESenseEvent.h"
#include "OSESense.h"
#include "OSESense_Hearing.h"
#include "OSESenseEvent_Hearing.h"
#include "OSESense_Damage.h"
#include "OSESenseEvent_Damage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESenseEvent)

//----------------------------------------------------------------------//
// UOSESenseEvent_Hearing
//----------------------------------------------------------------------//
UOSESenseEvent_Hearing::UOSESenseEvent_Hearing(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

FOSESenseID UOSESenseEvent_Hearing::GetSenseID() const
{
	return UOSESense::GetSenseID<UOSESense_Hearing>();
}

//----------------------------------------------------------------------//
// UOSESenseEvent_Damage
//----------------------------------------------------------------------//
FOSESenseID UOSESenseEvent_Damage::GetSenseID() const
{
	return UOSESense::GetSenseID<UOSESense_Damage>();
}

