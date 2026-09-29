// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "OSEPerceptionTypes.h"
#include "OSESense.h"
#include "OSESenseConfig.h"
#include "OSESense_Hearing.h"
#include "OSESenseConfig_Hearing.generated.h"

class FGameplayDebuggerCategory;
class UOSEPerceptionComponent;

UCLASS(meta = (DisplayName = "OSE Hearing config"))
class OSEPERCEPTION_API UOSESenseConfig_Hearing : public UOSESenseConfig
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", NoClear, config)
	TSubclassOf<UOSESense_Hearing> Implementation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense")
	float HearingRange;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", meta = (EditCondition = "bUseLoSHearing"))
	float LoSHearingRange;

	/** Warning: has significant runtime cost */
	UPROPERTY(EditDefaultsOnly, Category = "Sense", meta = (InlineEditConditionToggle))
	uint32 bUseLoSHearing : 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
	FOSESenseAffiliationFilter DetectionByAffiliation;

	virtual TSubclassOf<UOSESense> GetSenseImplementation() const override;

#if WITH_GAMEPLAY_DEBUGGER
	virtual void DescribeSelfToGameplayDebugger(const UOSEPerceptionComponent* PerceptionComponent, FGameplayDebuggerCategory* DebuggerCategory) const;
#endif // WITH_GAMEPLAY_DEBUGGER
};
