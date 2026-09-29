// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Perception/AISenseConfig.h"
#include "TATAISenseConfig_Sight.generated.h"

class UTATAISense_Sight;
class UOSEAISenseSharedConfigData;
enum class ETATEscalationState : uint8;

// Duplicate of FOSEPerAlertLevelSettings - When we move over to OSEPerception I don't want to have to remake
// all of the data. As this struct will likely survive the changes in the future it feels like the right thing to do.
USTRUCT(BlueprintType)
struct FTATPerEscalationLevelSettings
{
   GENERATED_BODY()

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

namespace TATAISenseConfig_Sight
{
   inline static constexpr FTATPerEscalationLevelSettings INVALID_SETTINGS;
}

UCLASS()
class TAT_API UTATAISenseConfig_Sight : public UAISenseConfig
{
   GENERATED_BODY()
public:
   UTATAISenseConfig_Sight();
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", NoClear, config)
   TSubclassOf<UTATAISense_Sight> Implementation;
   
   const FTATPerEscalationLevelSettings& GetSettingsForEscalation(const ETATEscalationState state) const;

   /** If not an InvalidRange (which is the default), we will always be able to see the target that has already been seen if they are within this range of their last seen location. */
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   float AutoSuccessRangeFromLastSeenLocation { 0.f };

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   FAISenseAffiliationFilter DetectionByAffiliation;

   UPROPERTY(EditDefaultsOnly, Category="Sense")
   UOSEAISenseSharedConfigData* SharedConfigData {nullptr};
   virtual TSubclassOf<UAISense> GetSenseImplementation() const override;
#if WITH_GAMEPLAY_DEBUGGER
   virtual void DescribeSelfToGameplayDebugger(const UAIPerceptionComponent* perceptionComponent, FGameplayDebuggerCategory* debuggerCategory) const override;
#endif // WITH_GAMEPLAY_DEBUGGER

protected:
   /** The values used by an AI depending on their escalation state **/
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sense", config)
   TMap<ETATEscalationState, FTATPerEscalationLevelSettings> _EscalationStateToSettings;
};
