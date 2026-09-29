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

#include "OSESenseConfig_SightFrustum.generated.h"

class FGameplayDebuggerCategory;
class UOSEPerceptionComponent;
class UOSESense_SightFrustum;

USTRUCT(BlueprintType)
struct FOSESightFrustumSettings
{
   GENERATED_USTRUCT_BODY()

   /** Maximum sight distance to notice a target. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
   float SightRadius = 0.0f;

   /** Maximum sight distance to see target that has been already seen. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
   float LoseSightRadius = 0.0f;

   /** How far to the side AI can see, in degrees. Use SetPeripheralVisionAngle to change the value at runtime.
    * The value represents the angle measured in relation to the forward vector, not the whole range. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 1.0, ClampMin = 1.0, UIMax = 180.0, ClampMax = 180.0, DisplayName = "PeripheralVisionHalfAngleDegrees"))
   float VisionHorizontalAngleDegrees = 22.5f;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 1.0, ClampMin = 1.0, UIMax = 180.0, ClampMax = 180.0, DisplayName = "PeripheralVisionHalfAngleDegrees"))
   float VisionVerticalAngleDegrees = 10.0f;

   /** Point of view move back distance for cone calculation. In conjunction with near clipping distance, this will act as a close by awareness and peripheral vision. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
   float PointOfViewBackwardOffset = 0.0f;

   /** Near clipping distance, to be used with point of view backward offset. Will act as a close by awareness and peripheral vision */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
   float NearClippingRadius = 0.0f;

   /** The pitch, used to make the frustum tilt up or down. 0.0 is parallel to the ground, negative is pointing down at the ground */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = -180.0, ClampMin = -180.0, UIMax = 180.0, ClampMax = 180.0))
   float FrustumPitch = -18.0;
};

UCLASS(meta = (DisplayName = "OSE Sight Frustum Config"))
class OSEPERCEPTION_API UOSESenseConfig_SightFrustum : public UOSESenseConfig
{
   GENERATED_UCLASS_BODY()

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", NoClear, config)
   TSubclassOf<UOSESense_SightFrustum> Implementation;

   /** The values used by an AI in the Neutral alert state */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   FOSESightFrustumSettings FrustumSettings;

   ///** */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   FOSESenseAffiliationFilter DetectionByAffiliation;

   /** If not an InvalidRange (which is the default), we will always be able to see the target that has already been seen if they are within this range of their last seen location. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   float AutoSuccessRangeFromLastSeenLocation;

   /** If not an InvalidRange (which is the default), we will always be able to get LOS to the target if they are within this range. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   float AutoSuccessLOSRange;

   //const FOSESightFrustumSettings& GetSettingsForAlertLevel(EAlertnessLevel AlertnessLevel) const;

   virtual TSubclassOf<UOSESense> GetSenseImplementation() const override;

#if WITH_EDITOR
   virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

#if WITH_GAMEPLAY_DEBUGGER
   virtual void DescribeSelfToGameplayDebugger(const UOSEPerceptionComponent* PerceptionComponent, FGameplayDebuggerCategory* DebuggerCategory) const;
#endif // WITH_GAMEPLAY_DEBUGGER
};

//static_assert((int32)EAlertnessLevel::MAX == 4, "If we're changing the alertness levels, we also need to edit UOSESenseConfig_SightFrustum's values to match");
