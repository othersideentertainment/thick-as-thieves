// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Utility/TATUtilityAITargetCache.h"

// ose
#include "AI/Utility/UtilityAIGoalComponent.h"

// ue4
#include "CoreMinimal.h"

// self
#include "TATUtilityAIGoalComponent.generated.h"

class ATATAIController;

UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class TAT_API UTATUtilityAIGoalComponent : public UUtilityAIGoalComponent
{
   GENERATED_BODY()

public:
   UTATUtilityAIGoalComponent();

   // from AActor
   virtual void BeginPlay() override;

   // when we set allowed enemy actor(s) we ignore all other actors.
   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   void AddAllowedEnemyActor(AActor* actor);
   // remove allowed enemy actor from our list
   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   void RemoveAllowedEnemyActor(AActor* actor);
protected:

   // from UUtilityAIComponent
   virtual bool _HasCachedPotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) const override;
   virtual void _CachePotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) override;
   virtual const TArray<FUtilityStateTarget>& _GetPotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) const override;
   virtual void _ResetPotentialTargetCache() override;

private:
   UPROPERTY(Transient)
   ATATAIController* _tatAIController = nullptr;

   UPROPERTY(Transient)
   FPotentialTargetsCache _potentialTargetsCache;
};
