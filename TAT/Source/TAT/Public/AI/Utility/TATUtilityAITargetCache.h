// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Utility/ConsiderationInput.h"
#include "AI/Utility/UtilityAITypes.h"

// ue4
#include "CoreMinimal.h"

// self
#include "TATUtilityAITargetCache.generated.h"

class ATATAIController;

USTRUCT()
struct TAT_API FPotentialTargetsGroup
{
   GENERATED_BODY()

public:
   FPotentialTargetsGroup()
      : TargetGroup()
   {
   }

   FPotentialTargetsGroup(const FGameplayTag& targetGroup)
      : TargetGroup(targetGroup)
   {
   }

   FGameplayTag TargetGroup;
   TArray<FUtilityStateTarget> Targets;
};

USTRUCT()
struct TAT_API FPotentialTargetsCache
{
   GENERATED_BODY()

public:
   // target cache
   bool HasCachedPotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) const;
   void CachePotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup, ATATAIController* aiController);
   const TArray<FUtilityStateTarget>& GetPotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) const;
   void ResetPotentialTargetCache();
   void SetWorld(UWorld* world);

   // allowed enemies
   void AddAllowedEnemyActor(AActor* actor, ATATAIController* aiController);
   void RemoveAllowedEnemyActor(AActor* actor);

   // smart objects
   void SetSmartObjects(TArray<FSmartObjectRequestResult>&& smartObjects);
   const TArray<FSmartObjectRequestResult>& GetSmartObjects() const { return _smartObjects; }

private:
   TMap<EUtilityStateTargeting, TArray<FPotentialTargetsGroup>> _potentialTargets;
   UPROPERTY(Transient)
   TArray<AActor*> _allowedEnemyActors;
   TArray<FSmartObjectRequestResult> _smartObjects;
   TWeakObjectPtr<UWorld> _world;

private:
   bool _IsAllowedToTargetEnemyActor(AActor* actor) const;
   bool _TryAddToCache(AActor* actor, const FGameplayTag& targetingGroup, TArray<FUtilityStateTarget>& targets) const;
   bool _TryAddToCache(const FSmartObjectRequestResult& smartObjectRequest, const FGameplayTag& targetingGroup, TArray<FUtilityStateTarget>& targets) const;
   bool _TryAddToCache(const AActor* owner, USmartObjectComponent* smartObjectComponent, const FGameplayTag& targetingGroup, TArray<FUtilityStateTarget>& targets) const;
   const FPotentialTargetsGroup* _FindPotentialTargets(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) const;
   FPotentialTargetsGroup* _FindPotentialTargets(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) { return const_cast<FPotentialTargetsGroup*>(const_cast<const FPotentialTargetsCache*>(this)->_FindPotentialTargets(targeting, targetingGroup)); }
};
