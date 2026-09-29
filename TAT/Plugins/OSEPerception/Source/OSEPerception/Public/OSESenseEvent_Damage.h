// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "OSESenseEvent.h"
#include "OSESense_Damage.h"
#include "OSESenseEvent_Damage.generated.h"

UCLASS()
class OSEPERCEPTION_API UOSESenseEvent_Damage : public UOSESenseEvent
{
	GENERATED_BODY()

public:	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sense")
	FOSEDamageEvent Event;
	
	virtual FOSESenseID GetSenseID() const override;

	FORCEINLINE FOSEDamageEvent GetDamageEvent()
	{
		Event.Compile();
		return Event;
	}
};
