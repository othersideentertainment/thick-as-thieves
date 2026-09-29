// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DeveloperSettings.h"

// ose
#include "AI/Alertness/DetectionEnums.h"

#include "TATAISettings.generated.h"

enum class ETATDifficulty : uint8;
//---------------------------------------------------------------------------------------
// UTATAISettings
//---------------------------------------------------------------------------------------
class UGameplayEffect;

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] AI Settings"))
class TAT_API UTATAISettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   // bp access
   UFUNCTION(BlueprintPure, Category = "TAT AI Settings")
   static UTATAISettings* GetTATAISettings() { return GetMutableDefault<UTATAISettings>(); }

   // C++ access
   static const UTATAISettings& Get() { return *GetDefault<UTATAISettings>(); }

   UPROPERTY(Config, EditDefaultsOnly, Category = "Gameplay Tags|Status", meta = (Categories = "Status"))
   FGameplayTag BamboozledTag;

   // these are tags applied to actors that the spotlights / security cameras / vril beams should explicitly not see
   UPROPERTY(Config, EditDefaultsOnly, Category = "Gameplay Tags|Perception|Devices")
   FGameplayTagContainer DevicesDoNotSeeTags;

   // these are smart objects that should be known about by AI no matter where they are in the world
   UPROPERTY(Config, EditDefaultsOnly, Category = "Gameplay Tags|Smart Objects")
   FGameplayTagQuery AlwaysKnownSmartObjectsActivityQuery;

   // these are the states are the ones where we don't want to evaluate a goal at all, we just want to drive our tree straight into a direct leaf in response to a tag
   // in general these are things like dead, combat disabled, inside of smoke, etc
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Gameplay Tags|Forced States")
   FGameplayTagContainer ForcedStateTags;

   UPROPERTY(Config, EditDefaultsOnly, Category = "TATAIStateWorldSubsystem")
   float DistanceForMaxAlertnessCalculation = 2000.0f;

   // extra distance that an AI has to be from the player to no longer be counted toward max alertness
   UPROPERTY(Config, EditDefaultsOnly, Category = "TATAIStateWorldSubsystem")
   float ExtraDistanceForMaxAlertnessCalculationBuffer = 1000.0f;
   
   UPROPERTY(Config, EditDefaultsOnly, Category = "Gameplay Tags|Interaction", meta = (Categories = "Ability.Interact"))
   FGameplayTag ForcedInteractionGameplayTag;

   // Used to apply a gameplay effect on a character when they are detected / undetected by an AI agent
   UPROPERTY(Config, EditAnywhere, Category = "Gameplay Effects|Detection")
   TMap<EActorDetectionState, TSubclassOf<UGameplayEffect>> DetectionStateToGameplayEffect;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Gameplay Tags|Interaction")
   FName ShareTargetDataStimTag;

   /// What collision channel we should use for checking if a noise stim is heard by a listener
   UPROPERTY(Config, EditDefaultsOnly, Category = "Perception")
   TEnumAsByte<ECollisionChannel> NoiseStimTraceChannel = ECC_Visibility;

   // Used to simulate the muffling of sound through a closed audio portal. 
   UPROPERTY(Config, EditDefaultsOnly, Category = "Perception")
   float DistanceMultiplierWhenPropagatingThroughClosedPortal { 2.f };

   // Default interaction prompt text used for clue-giving AI. Can be overriden by individual clues
   // via FTATNPCDialogueClue::OverrideInteractionPrompt.
   UPROPERTY(Config, EditDefaultsOnly, Category = "Clues")
   FText AIClueInteractionPrompt;

   // Initial AI clue-giving allowed state for clue components spawned by spawners.
   UPROPERTY(Config, EditDefaultsOnly, Category = "Clues")
   bool InitialClueSpawnerAIClueGivingEnabled = false;
   
   UPROPERTY(Config, EditDefaultsOnly, Category="Difficulty")
   TMap<ETATDifficulty, int> DifficultyToVigilantLoopCount;

   static UDataTable* GetHearingStimSettings() {
      return Get()._HearingEventStimSettings.LoadSynchronous();
   }
private:
   UPROPERTY(Config, EditDefaultsOnly, Category = "Sense", meta = (RowType = "/Script/TAT.TATHearingEventStimSettings"))
   TSoftObjectPtr<UDataTable> _HearingEventStimSettings = nullptr;
};
