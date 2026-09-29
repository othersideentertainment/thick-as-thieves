// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "AI/Alertness/AlertnessEnums.h"

// ue5
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "TATDetectionSettingsAsset.generated.h"

class UCurveFloat;

USTRUCT(Meta = (DisplayName = "Detection Ramp Settings (crouch)"))
struct TAT_API FTATDetectionRampCrouchSettings
{
   GENERATED_BODY()

public:
   // Minimum Distance at which we apply the crouch ramp modifier
   UPROPERTY(EditDefaultsOnly, Meta = (ClampMin = "0.0", UIMin = "0.0"))
   float MinimumDistance = 1000.0f;

   // Minimum Distance at which we apply the crouch ramp modifier
   UPROPERTY(EditDefaultsOnly, Meta = (ClampMin = "0.0", UIMin = "0.0"))
   float DetectionMultiplier = 1.0f;
};

USTRUCT(Meta = (DisplayName = "Detection Ramp Settings"))
struct TAT_API FTATDetectionRampSettings
{
   GENERATED_BODY()

public:
   // How close do we consider the "critical range"?
   UPROPERTY(EditDefaultsOnly, meta = (Units = "cm"))
   float CriticalRange = 0.0;

   // How long should detection take for this the current detection level within the "critical range" ?
   UPROPERTY(EditDefaultsOnly)
   float CriticalTimeSeconds = 0.0;

   // Add this many seconds per meter away the actor is from the AI outside of the critical range
   UPROPERTY(EditDefaultsOnly)
   float AgnosticTimeSecondsPerMeter = 0.0;

   /// Multiplier for detection speed when the player has a weapon equipped
   UPROPERTY(EditAnywhere)
   float WeaponEquippedDetectionMultiplier = 1.5f;

   UPROPERTY(EditAnywhere)
   TObjectPtr<UCurveFloat> HeightDifferenceToStrengthMultiplierCurve = nullptr;

   /// Multiplier for detection speed when the player is >= MinimumDistance
   UPROPERTY(EditAnywhere)
   FTATDetectionRampCrouchSettings CrouchSettings;
};

USTRUCT()
struct TAT_API FTATPerAlertnessDetectionMultiplier
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly)
   float Neutral = 1;

   UPROPERTY(EditDefaultsOnly)
   float Suspicious = 1;

   UPROPERTY(EditDefaultsOnly)
   float Alerted = 1;

   UPROPERTY(EditDefaultsOnly)
   float Combat = 1;

   float Get(EAlertnessLevel alertness) const
   {
      switch (alertness)
      {
      case EAlertnessLevel::Neutral:
         return Neutral;
      case EAlertnessLevel::Suspicious:
         return Suspicious;
      case EAlertnessLevel::Alerted:
         return Alerted;
      case EAlertnessLevel::Combat:
         return Combat;
      default:
         checkNoEntry();
         return 1;
      }
   }
};

USTRUCT()
struct TAT_API FTATTagDetectionRampModifier
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly)
   FGameplayTag Tag;

#if WITH_EDITORONLY_DATA
   UPROPERTY(EditDefaultsOnly)
   FString Comment;
#endif

   UPROPERTY(EditDefaultsOnly)
   FTATPerAlertnessDetectionMultiplier Multipliers;
};

USTRUCT(Meta = (DisplayName = "Detection Ramp Settings"))
struct TAT_API FTATDetectionDecaySettings
{
   GENERATED_BODY()

public:
   // How long should we stay in the max detection level before we allow decay?
   UPROPERTY(EditDefaultsOnly)
   float MinSecondsInCurrentDetectionLevelBeforeDecay = 3.0;

   // How long since we've seen the actor should it be before we allow decay?
   UPROPERTY(EditDefaultsOnly)
   float MinSecondsSinceVisibleBeforeDecay = 2.0f;

   // How long should it take to decay through a single state?
   UPROPERTY(EditDefaultsOnly)
   float DetectionDecaySeconds = 3.5;
   
   UPROPERTY(EditDefaultsOnly)
   float DecayMultiplierIfNPCCollidingWithGeometry = 1.f;

   // We look up the distance in this curve so designers can tweak decay rate by distance
   // X axis is distance, Y axis is the multiplier ( < 1 to slow it down, > 1 to speed it up)
   UPROPERTY(EditDefaultsOnly)
   UCurveFloat* DecayDistanceCurve = nullptr;
};

USTRUCT(Meta = (DisplayName = "Detection Settings"))
struct TAT_API FTATDetectionSettings
{
   GENERATED_BODY()

public:
   FTATDetectionSettings();

   UPROPERTY(EditAnywhere, Category = "Ramp")
   TMap<EAlertnessLevel, FTATDetectionRampSettings> RampSettings;

   UPROPERTY(EditAnywhere, Category = "Ramp", meta = (TitleProperty = "{Tag} ({Comment})"), DisplayName="Target Gameplay Tag Detection Modifiers")
   TArray<FTATTagDetectionRampModifier> GameplayTagRampModifiers;
   
   UPROPERTY(EditAnywhere, Category = "Ramp", meta = (TitleProperty = "{Tag} ({Comment})"), DisplayName="Source Gameplay Tag Detection Modifiers")
   TArray<FTATTagDetectionRampModifier> SourceGameplayTagRampModifiers;

   UPROPERTY(EditAnywhere, Category = "Decay")
   FTATDetectionDecaySettings DecaySettings;
};

UCLASS()
class TAT_API UTATDetectionSettingsAsset : public UDataAsset
{
	GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, Meta = (ShowOnlyInnerProperties))
   FTATDetectionSettings DetectionSettings;
};
