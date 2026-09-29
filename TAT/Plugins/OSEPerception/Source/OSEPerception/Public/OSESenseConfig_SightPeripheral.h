// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#pragma once

// unreal
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"


// ose
//#include "AI/Perception/OSEAISense_Sight.h"
//#include "AI/Alertness/AlertnessEnums.h"
#include "OSESenseConfig.h"
#include "OSEPerceptionTypes.h"
#include "OSESense.h"

#include "OSESenseConfig_SightPeripheral.generated.h"

class FGameplayDebuggerCategory;
class UOSEPerceptionComponent;
class UOSESense_SightPeripheral;


UCLASS(meta = (DisplayName = "OSE Sight Peripheral Config"))
class OSEPERCEPTION_API UOSESenseConfig_SightPeripheral : public UOSESenseConfig
{
   GENERATED_UCLASS_BODY()

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", NoClear, config)
   TSubclassOf<UOSESense_SightPeripheral> Implementation;


   /** Maximum sight distance to notice a target. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
   float SightRadius = 0.0f;

   /** Maximum sight distance to see target that has been already seen. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
   float LoseSightRadius = 0.0f;

   /** Point of view move back distance for cone calculation. In conjunction with near clipping distance, this will act as a close by awareness and peripheral vision. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
   float PointOfViewBackwardOffset = 0.0f;

   ///** */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   FOSESenseAffiliationFilter DetectionByAffiliation;

   /** If not an InvalidRange (which is the default), we will always be able to see the target that has already been seen if they are within this range of their last seen location. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   float AutoSuccessRangeFromLastSeenLocation;

   /** If not an InvalidRange (which is the default), we will always be able to get LOS to the target if they are within this range. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   float AutoSuccessLOSRange;

   //const FOSESightPeripheralSettings& GetSettingsForAlertLevel(EAlertnessLevel AlertnessLevel) const;

   virtual TSubclassOf<UOSESense> GetSenseImplementation() const override;

#if WITH_EDITOR
   virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

#if WITH_GAMEPLAY_DEBUGGER
   virtual void DescribeSelfToGameplayDebugger(const UOSEPerceptionComponent* PerceptionComponent, FGameplayDebuggerCategory* DebuggerCategory) const;
#endif // WITH_GAMEPLAY_DEBUGGER
};
