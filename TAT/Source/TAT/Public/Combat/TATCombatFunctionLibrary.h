// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATCombatFunctionLibrary.generated.h"

class UOSESyncedAnimationDataAsset;
class UTATCombatComponent;

UCLASS()
class TAT_API UTATCombatFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintPure, Category = "TAT|Combat")
   static bool IsDefenderShielded(const AActor* defender);

   UFUNCTION(BlueprintPure, Category = "TAT|Combat")
   static bool IsDefenderParrying(const AActor* attacker, const AActor* defender);
   
   UFUNCTION(BlueprintPure, Category = "TAT|Combat")
   static bool IsDefenderInPositionToBlockAttacker(const AActor* attacker, const AActor* defender);

   UFUNCTION(BlueprintPure, Category = "TAT|Combat")
   static bool IsDefenderBlocking(const AActor* attacker, const AActor* defender);

   UFUNCTION(BlueprintPure, Category = "TAT|Combat")
   static bool IsDefenderVulnerableToCounterByAttacker(const AActor* attacker, const AActor* defender);
   
   UFUNCTION(BlueprintPure, Category = "Combat")
   static bool IsBlocking(const AActor* actor);

   UFUNCTION(BlueprintPure, Category = "TAT|Combat")
   static bool IsPerformingChargeAttack(const AActor* actor);

   UFUNCTION(BlueprintPure, Category = "TAT|Combat")
   static bool IsChargingMeleeAttack(const AActor* actor);

   UFUNCTION(BlueprintPure, Category = "TAT|Combat")
   static bool IsPerformingShove(const AActor* actor);

   UFUNCTION(BlueprintPure, Category = "TAT|Combat")
   static bool CanBeSneakAttackedFromPosition(const AActor* defender, FVector attackerPosition);

   UFUNCTION(BlueprintCallable, Category = "TAT|Combat")
   static AActor* FindStealthTakedownTarget(const AActor* attacker, const TArray<UOSESyncedAnimationDataAsset*>& takedownSyncedAnimations, float traceDistance, float traceConeHalfAngle, FCollisionProfileName traceProfile);

   UFUNCTION(BlueprintCallable, Category = "TAT|Combat")
   static bool CanPerformStealthTakedownAgainst(const AActor* attacker, const AActor* possibleTarget, const TArray<UOSESyncedAnimationDataAsset*>& takedownSyncedAnimations);

   UFUNCTION(BlueprintCallable, Category = "TAT|Combat")
   static bool PerformStealthTakedown(const AActor* attacker, const AActor* defender, FGameplayTag takedownEventTag, const TArray<UOSESyncedAnimationDataAsset*>& takedownSyncedAnimations);

   static bool IsValidTakedownDefender(const AActor* defender);

   // Takse a dodging-pawn and FTATGameplayAbilityTargetData_Dodge handle, returns end location + whether it was a stationary dodge (i.e. backstep)
   UFUNCTION(BlueprintPure, Category = "TAT|Combat|Dodge")
   static void GetDodgeInfoFromTargetData(const APawn* dodgingPawn, const FGameplayAbilityTargetDataHandle& targetData, bool& success, FVector& dodgeEndLocation, FVector& dodgeDirection, bool& isBackstep);

   UFUNCTION(BlueprintPure, Category = "TAT|Combat")
   static UTATCombatComponent* GetTATCombatComponent(const AActor* actor);
};
