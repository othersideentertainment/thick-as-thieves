// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "AI/Alertness/AlertnessEnums.h"
#include "AI/Alertness/DetectionEnums.h"

// ue4
#include "GameplayTagContainer.h"
#include "Engine/DeveloperSettings.h"

#include "AlertnessSettings.generated.h"

USTRUCT(BlueprintType, Meta = (DisplayName = "Alertness Settings"))
struct OSEAI_API FAlertnessSettings
{
   GENERATED_BODY()

public:

   /// How long the AI will be able to know your location after losing visibility of you as a player
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (ClampMin = "0.0", UIMin = "0.0"))
   float NoVisibilityLocationSecondsPlayers = 4.0f;

   /// How long the AI will be able to know your location after losing visibility of another AI
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (ClampMin = "0.0", UIMin = "0.0"))
   float NoVisibilityLocationSecondsAI = 1.0f;
};

USTRUCT(BlueprintType, Meta = (DisplayName = "Alertness Transition Settings"))
struct OSEAI_API FAlertnessTransitionSettings
{
   GENERATED_BODY()

public:
   // Window (in seconds) where an AI can enter a transition animation behavior after
   // their alertness level has increased.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (ClampMin = "0.0", UIMin = "0.0"))
   float AlertnessTransitionAnimationWindow = 3.0f;
};

USTRUCT(BlueprintType, Meta = (DisplayName = "Forgetness Settings"))
struct OSEAI_API FForgetnessSettings
{
   GENERATED_BODY()

public:
   // We must be at this level or lower to forget an actor
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   EAlertnessLevel ForgetActorMaxAlertnessLevel = EAlertnessLevel::Neutral;

   // We must be at this level or lower to forget an actor
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   EActorDetectionState ForgetActorMaxDetectionState = EActorDetectionState::Identifying;

   // We must be at this detection level or lower to forget an actor
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float ForgetActorMaxDetectionValue = 0.0f;

   /// How long after no updates to ActorKnowledge we forget about the actor,
   /// assuming other alertness level & detection criteria are met.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (ClampMin = "0.0", UIMin = "0.0"))
   float ForgetActorsCutoff = 10.0f;
};

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[OSE] Alertness Settings"))
class OSEAI_API UAlertnessSettingsConfig : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintPure, Category = "OSE|AI|Settings|Alertness")
   static const FAlertnessSettings& GetAlertnessSettings() { return GetDefault<UAlertnessSettingsConfig>()->AlertnessSettings; }
   UFUNCTION(BlueprintPure, Category = "OSE|AI|Settings|AlertnessTransition")
   static const FAlertnessTransitionSettings& GetAlertnessTransitionSettings() { return GetDefault<UAlertnessSettingsConfig>()->AlertnessTransitionSettings; }
   UFUNCTION(BlueprintPure, Category = "OSE|AI|Settings|Forgetness")
   static const FForgetnessSettings& GetForgetnessSettings() { return GetDefault<UAlertnessSettingsConfig>()->ForgetnessSettings; }
   
   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Meta = (DisplayName = "Alertness Settings"))
   FAlertnessSettings AlertnessSettings;

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Meta = (DisplayName = "Alertness Transition Settings"))
   FAlertnessTransitionSettings AlertnessTransitionSettings;

   UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Meta = (DisplayName = "Forgetness Settings"))
   FForgetnessSettings ForgetnessSettings;
};
