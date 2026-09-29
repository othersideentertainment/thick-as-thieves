// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#pragma once

#include "CoreMinimal.h"

// ue
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"

#include "TATCombatSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] Combat Settings"))
class TAT_API UTATCombatSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   // for bp
   UFUNCTION(BlueprintPure, Category = "TAT Combat Settings")
   static UTATCombatSettings* GetCombatSettings() { return GetMutableDefault<UTATCombatSettings>(); }
   
   // for C++
   static const UTATCombatSettings& Get() { return *GetDefault<UTATCombatSettings>(); }

public:
   // attacker event tags
   UPROPERTY(Config, EditDefaultsOnly, DisplayName = "BlockEventTag", Category = "GameplayTags|Combat|Attacker")
   FGameplayTag AttackerBlockEventTag;
   UPROPERTY(Config, EditDefaultsOnly, DisplayName = "ShieldEventTag", Category = "GameplayTags|Combat|Attacker")
   FGameplayTag AttackerShieldEventTag;
   UPROPERTY(Config, EditDefaultsOnly, DisplayName = "ParryEventTag", Category = "GameplayTags|Combat|Attacker")
   FGameplayTag AttackerParryEventTag;
   // defender event tags
   UPROPERTY(Config, EditDefaultsOnly, DisplayName = "BlockEventTag", Category = "GameplayTags|Combat|Defender")
   FGameplayTag DefenderBlockEventTag;
   UPROPERTY(Config, EditDefaultsOnly, DisplayName = "ShieldEventTag", Category = "GameplayTags|Combat|Defender")
   FGameplayTag DefenderShieldEventTag;
   UPROPERTY(Config, EditDefaultsOnly, DisplayName = "ParryEventTag", Category = "GameplayTags|Combat|Defender")
   FGameplayTag DefenderParryEventTag;
   // combat state query tags
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags|Combat")
   FGameplayTag IsBlockingTag;

   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags|Combat")
   FGameplayTag IsShieldedTag;

   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags|Combat")
   FGameplayTag IsParryingTag;

   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags|Combat")
   FGameplayTag IsPerformingMeleeAttackTag;
   
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags|Combat")
   FGameplayTag IsPerformingChargeAttackTag;

   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags|Combat")
   FGameplayTag IsChargingMeleeAttackTag;

   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags|Combat")
   FGameplayTag IsPerformingShoveTag;

   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags|Combat")
   FGameplayTag IsVulnerableToCounterTag;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Combat|Blocking")
   bool CanBlockChargeAttacks = false;

   // Determines how charged a heavy attack must be to break through a target's block
   UPROPERTY(Config, EditDefaultsOnly, Category = "Combat|Blocking", meta = (UIMin = 0, ClampMin = 0, UIMax = 100, ClampMax = 100, Units = "percent", EditCondition = "CanBlockChargeAttacks"))
   float RequiredChargeAttackPercentageToNegateBlock = 100;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Combat|Blocking")
   bool CanBlockShoves = true;

   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Dodge")
   float MovingDodgeMagnitude = 275.f;

   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Dodge")
   float BackstepDodgeMagnitude = 100.f;
};
