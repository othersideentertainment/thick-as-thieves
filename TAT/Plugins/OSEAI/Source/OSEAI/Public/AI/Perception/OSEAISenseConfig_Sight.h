// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// unreal
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Templates/SubclassOf.h"
#include "Perception/AIPerceptionTypes.h"
#include "Perception/AISense.h"
#include "Perception/AISenseConfig.h"

// ose
#include "AI/Perception/OSEAISense_Sight.h"
#include "AI/Alertness/AlertnessEnums.h"

#include "OSEAISenseConfig_Sight.generated.h"

class UOSEAISenseSharedConfigData;
class FGameplayDebuggerCategory;
class UAIPerceptionComponent;

USTRUCT(BlueprintType)
struct FOSEPerAlertLevelSettings
{
   GENERATED_USTRUCT_BODY()

   /** Maximum sight distance to notice a target. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
   float SightRadius = 0.0f;

   /** Maximum sight distance to see target that has been already seen. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
   float LoseSightRadius = 0.0f;
   
   /** The minimum distance that the sight can ever be reduced to. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
   float MinimumSightRadius = 0.0f;

   /** How far to the side AI can see, in degrees. Use SetPeripheralVisionAngle to change the value at runtime.
    * The value represents the angle measured in relation to the forward vector, not the whole range. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0, UIMax = 180.0, ClampMax = 180.0, DisplayName = "PeripheralVisionHalfAngleDegrees"))
   float PeripheralVisionAngleDegrees = 0.0f;

   /** Point of view move back distance for cone calculation. In conjunction with near clipping distance, this will act as a close by awareness and peripheral vision. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
   float PointOfViewBackwardOffset = 0.0f;

   /** Near clipping distance, to be used with point of view backward offset. Will act as a close by awareness and peripheral vision */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.0, ClampMin = 0.0))
   float NearClippingRadius = 0.0f;

   /** The aspect ratio, used to make the frustum wider or taller. 1.0 is square */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = 0.01, ClampMin = 0.01))
   float FrustumAspectRatio = 2.2f;

   /** The pitch, used to make the frustum tilt up or down. 0.0 is parallel to the ground, negative is pointing down at the ground */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config, meta = (UIMin = -180.0, ClampMin = -180.0, UIMax = 180.0, ClampMax = 180.0))
   float FrustumPitch = -18.0;
};

UCLASS(meta = (DisplayName = "OSE AI Sight config"))
class OSEAI_API UOSEAISenseConfig_Sight : public UAISenseConfig
{
   GENERATED_UCLASS_BODY()

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", NoClear, config)
   TSubclassOf<UOSEAISense_Sight> Implementation;

   /** The values used by an AI in the Neutral alert state */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   FOSEPerAlertLevelSettings NeutralValues;
   
   /** The values used by an AI in the Suspicious alert state */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   FOSEPerAlertLevelSettings SuspiciousValues;

   /** The values used by an AI in the Alerted alert state */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   FOSEPerAlertLevelSettings AlertedValues;

   /** The values used by an AI in the Combat alert state */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   FOSEPerAlertLevelSettings CombatValues;

   /** */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   FAISenseAffiliationFilter DetectionByAffiliation;

   UPROPERTY(EditDefaultsOnly, Category="Sense")
   UOSEAISenseSharedConfigData* SharedConfigData {nullptr};

   /** If not an InvalidRange (which is the default), we will always be able to see the target that has already been seen if they are within this range of their last seen location. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   float AutoSuccessRangeFromLastSeenLocation;

   const FOSEPerAlertLevelSettings& GetSettingsForAlertLevel(EAlertnessLevel alertnessLevel) const;

   virtual TSubclassOf<UAISense> GetSenseImplementation() const override;

#if WITH_EDITOR
   virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

#if WITH_GAMEPLAY_DEBUGGER
   virtual void DescribeSelfToGameplayDebugger(const UAIPerceptionComponent* PerceptionComponent, FGameplayDebuggerCategory* DebuggerCategory) const;
#endif // WITH_GAMEPLAY_DEBUGGER
};

static_assert((int32)EAlertnessLevel::MAX == 4, "If we're changing the alertness levels, we also need to edit UOSEAISenseConfig_Sight's values to match");
