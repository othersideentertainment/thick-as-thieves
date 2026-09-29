// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#pragma once

// unreal
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"


// ose
//#include "AI/Perception/OSEAISense_Hearing.h"
//#include "AI/Alertness/AlertnessEnums.h"
#include "OSESenseConfig.h"
#include "OSEPerceptionTypes.h"
#include "OSESense.h"

#include "OSESenseConfig_HearingContinuous.generated.h"

class FGameplayDebuggerCategory;
class UOSEPerceptionComponent;
class UOSESense_HearingContinuous;


UCLASS(meta = (DisplayName = "OSE Hearing Continuous Config"))
class OSEPERCEPTION_API UOSESenseConfig_HearingContinuous : public UOSESenseConfig
{
   GENERATED_UCLASS_BODY()

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", NoClear, config)
   TSubclassOf<UOSESense_HearingContinuous> Implementation;


   /** Maximum Hearing distance to notice a target. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
   float HearingRadius = 0.0f;

   /** Maximum Hearing distance to see target that has been already seen. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
   float LoseHearingRadius = 0.0f;

   /** The percentage of MAx Intensity sound below which the entity cannot hear a sound. So if the percent is 0.5, at 50% of a gunshot a foot step would no longer be heard*/
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
   float PercentMaskingSound = 0.0f;

   /** The minimum location error to apply to the stimulus location. At zero the error will be zero for near by sounds*/
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
   float MinErrorRadius = 0.0f;

   /** The minimum location error to apply to the stimulus location. At zero the error will be zero for far off sounds*/
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
   float MaxErrorRadius = 0.0f;

   ///** */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   FOSESenseAffiliationFilter DetectionByAffiliation;

   /** If not an InvalidRange (which is the default), we will always be able to see the target that has already been seen if they are within this range of their last seen location. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   float AutoSuccessRangeFromLastSeenLocation;

   //const FOSEHearingPeripheralSettings& GetSettingsForAlertLevel(EAlertnessLevel AlertnessLevel) const;

   virtual TSubclassOf<UOSESense> GetSenseImplementation() const override;

#if WITH_EDITOR
   virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

#if WITH_GAMEPLAY_DEBUGGER
   virtual void DescribeSelfToGameplayDebugger(const UOSEPerceptionComponent* PerceptionComponent, FGameplayDebuggerCategory* DebuggerCategory) const;
#endif // WITH_GAMEPLAY_DEBUGGER
};
