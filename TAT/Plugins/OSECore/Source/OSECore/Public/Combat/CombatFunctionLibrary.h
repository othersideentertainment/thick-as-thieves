// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Combat/CombatTypes.h"

#include "CombatFunctionLibrary.generated.h"

UCLASS()
class OSECORE_API UCombatFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintPure, Category = "Combat")
   static bool IsDefenderInFrontOfAttacker(const AActor* attacker, const AActor* defender, float dotTolerance = -1.0f);

   UFUNCTION(BlueprintPure, Category = "Combat")
   static bool IsLocationInFrontOfAttacker(const AActor* attacker, const FVector& location, float dotTolerance = -1.0f);

   UFUNCTION(BlueprintCallable, Category = "Combat")
   static bool DoesAttackerHaveHitPathToDefender(const AActor* attacker, const AActor* defender, float radius, EOSECombatDefenderTraceLogic traceLogic = EOSECombatDefenderTraceLogic::TraceToCenter);

   UFUNCTION(BlueprintCallable, Category = "Combat")
   static bool DoesAttackerHaveHitPathToDefenderLocation(const AActor* attacker, const AActor* defender, const FVector& location, float radius);

   UFUNCTION(BlueprintPure, Category = "Combat")
   static bool IsHeavyAttackReadied(const AActor* actor);
};
