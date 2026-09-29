// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose

// ue4
#include "GameplayTagContainer.h"
#include "Engine/CollisionProfile.h"
#include "Engine/DeveloperSettings.h"

#include "CombatSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[OSE] Combat Settings"))
class OSECORE_API UCombatSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   // for bp
   UFUNCTION(BlueprintPure, Category = "OSE Combat Settings")
   static UCombatSettings* GetCombatSettings() { return GetMutableDefault<UCombatSettings>(); }

   // for C++
   static const UCombatSettings& Get() { return *GetDefault<UCombatSettings>(); }

   // TODO: probably move most of this to the TAT level, since that is where it is used

   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags|Combat")
   FGameplayTag AttackerHitEventTag;
   // defender event tags
   UPROPERTY(Config, EditDefaultsOnly, DisplayName = "DefendEventTag", Category = "GameplayTags|Combat|Defender")
   FGameplayTag DefenderEventTag;

   // geometry hit event tag
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags|Combat")
   FGameplayTag GeometryHitEventTag;

   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags|Combat")
   FGameplayTag IsHeavyAttackReadiedTag;

   // combat damage calculation tags
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags|Combat")
   FGameplayTag DamageEffectMagnitudeTag;

   // combat disabled status tag
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags|Combat")
   FGameplayTag StatusCombatDisabled;

   // ranged combat tags
   UPROPERTY(Config, EditDefaultsOnly, DisplayName = "ProjectileSpawn", Category = "GameplayTags|Combat|Ranged")
   FGameplayTag RangedProjectileSpawn;
   UPROPERTY(Config, EditDefaultsOnly, DisplayName = "ProjectileReload", Category = "GameplayTags|Combat|Ranged")
   FGameplayTag RangedProjectileReload;

   // Which trace profile should we use when we swing our weapon?
   UPROPERTY(Config, EditDefaultsOnly, Category = "Combat")
   FCollisionProfileName CombatSwingTraceProfile;

   // Which trace profile should we use when tracing a path to potential defenders?
   UPROPERTY(Config, EditDefaultsOnly, Category = "Combat")
   FCollisionProfileName CombatHitPathTraceProfile;
   // How big of a radius should we use when tracing a path between attackers and defenders?
   UPROPERTY(Config, EditDefaultsOnly, Category = "Combat")
   float CombatAttackerToDefenderPathTraceSphereRadius = 10.0f;

   // How far should the character travel when dodging?
   UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Dodge")
   float MovingDodgeMagnitude = 275.0f;
      
   //We're not supporting backstep right now. Leaving it here in case we want it in the future
   //UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Dodge")
   float BackstepDodgeMagnitude = 100.0f;;
};
