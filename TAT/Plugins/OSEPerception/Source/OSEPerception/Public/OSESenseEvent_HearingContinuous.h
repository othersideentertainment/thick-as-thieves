// Copyright Epic Games, Inc. All Rights Reserved.
// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#pragma once

#include "CoreMinimal.h"
#include "OSESenseEvent.h"
#include "OSESense_HearingContinuous.h"
#include "OSESenseEvent_HearingContinuous.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class OSEPERCEPTION_API UOSESenseEvent_HearingContinuous : public UOSESenseEvent
{
	GENERATED_BODY()
	
public:
   UOSESenseEvent_HearingContinuous(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
   virtual FOSESenseID GetSenseID() const override;

   FOSEHearingEvent Event;
};
