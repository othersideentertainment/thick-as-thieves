// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Utility/UtilityAITokenOwner.h"

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Engine/DataTable.h"
#include "Engine/DeveloperSettings.h"

#include "OSEAISettings.generated.h"

//---------------------------------------------------------------------------------------
// UOSEAISettings
//---------------------------------------------------------------------------------------

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[OSE] AI Settings"))
class OSEAI_API UOSEAISettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   static const UOSEAISettings& Get() { return *GetDefault<UOSEAISettings>(); }

   UPROPERTY(Config, EditDefaultsOnly, Category = "Debug")
   bool ShowingAIDebugHUD = false;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Gameplay Tags|Knowledge|Detection")
   FGameplayTagContainer TargetBlockDetectionTags;

   // AI with these tags can be targets of investigations using their last known locations, but not targets of direction actions eg combat
   UPROPERTY(Config, EditDefaultsOnly, Category = "Gameplay Tags|Knowledge|Detection")
   FGameplayTagContainer NotActionableTargetActorTags;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Gameplay Tags|Perception|Sight")
   FGameplayTagContainer DoNotSeeActorTags;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Gameplay Tags|Utility Behavior", meta = (Categories = "AI.BehaviorTree.InjectionTags"))
   FGameplayTag UtilityBehaviorTreeInjectionTag;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Gameplay Tags|Utility Behavior")
   TSoftObjectPtr<UBehaviorTree> DefaultInjectedUtilityBehaviorTree;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Gameplay Tags|Utility State", meta = (Categories = "AI.State.Metadata"))
   FGameplayTag AggressiveBehaviorTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Gameplay Tags|Utility State", meta = (Categories = "AI.State.Metadata"))
   FGameplayTag DefenselessBehaviorTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Gameplay Tags|Utility State", meta = (Categories = "AI.State.Metadata"))
   FGameplayTag TransitionalStateTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Gameplay Tags|Utility Behavior")
   FOSEAITokenInfo MeleeCombatAIToken;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Gameplay Tags|Utility Behavior", meta = (Categories = "AI.Token"))
   FGameplayTag SmartObjectAITokenTag;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Stims")
   float MaxStimAge = 10.0f;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Stims", meta = (RowType = "/Script/OSEAI.OSEStimSettings"))
   TSoftObjectPtr<UDataTable> StimSettingsTable;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Stims")
   FName TouchStimId;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Stims")
   FName DamageStimId;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Stims")
   FName TeamStimId;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Utility")
   float MinStateScore = 0.01f;
   UPROPERTY(Config, EditDefaultsOnly, Category = "Utility")
   float MinConditionalScore = 0.001f;
};
