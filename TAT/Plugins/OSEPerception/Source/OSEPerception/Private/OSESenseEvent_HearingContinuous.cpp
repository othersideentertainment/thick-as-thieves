// Copyright Epic Games, Inc. All Rights Reserved.
// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "OSESenseEvent_HearingContinuous.h"
#include "OSESense_HearingContinuous.h"

UOSESenseEvent_HearingContinuous::UOSESenseEvent_HearingContinuous(const FObjectInitializer& ObjectInitializer)
   : Super(ObjectInitializer)
{

}

FOSESenseID UOSESenseEvent_HearingContinuous::GetSenseID() const
{
   return UOSESense::GetSenseID<UOSESense_HearingContinuous>();
}
