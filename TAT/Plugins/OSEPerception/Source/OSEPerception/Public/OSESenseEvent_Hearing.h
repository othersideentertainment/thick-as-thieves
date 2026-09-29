// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "OSESense_Hearing.h"
#include "OSESenseEvent.h"
#include "OSESenseEvent_Hearing.generated.h"

UCLASS()
class OSEPERCEPTION_API UOSESenseEvent_Hearing : public UOSESenseEvent
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
	FOSENoiseEvent Event;

public:
	UOSESenseEvent_Hearing(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual FOSESenseID GetSenseID() const override;
	
	FORCEINLINE FOSENoiseEvent GetNoiseEvent()
	{
		Event.Compile();
		return Event;
	}
};
