// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Utility/TATUtilityAITargetCache.h"

// ose
#include "AI/Utility/UtilityAIBehaviorComponent.h"

// ue
#include "CoreMinimal.h"

// self
#include "TATUtilityAIBehaviorComponent.generated.h"

class ATATAIController;

USTRUCT()
struct FInjectedBehaviorEntry
{
   GENERATED_BODY()

   FGameplayTag InjectionTag;
   
   UPROPERTY(Transient)
   TArray<UUtilityBehaviorSet*> Behaviors;
};

UCLASS(Blueprintable, meta=(BlueprintSpawnableComponent))
class TAT_API UTATUtilityAIBehaviorComponent : public UUtilityAIBehaviorComponent
{
   GENERATED_BODY()

public:
   UTATUtilityAIBehaviorComponent();

   // when we set allowed enemy actor(s) we ignore all other actors.
   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   void AddAllowedEnemyActor(AActor* actor);
   // remove allowed enemy actor from our list
   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   void RemoveAllowedEnemyActor(AActor* actor);
   
   void AddInjectedBehaviors(const FGameplayTag& gameplayTag, const TArray<UUtilityBehaviorSet*>& behaviorsToAdd);
   void RemoveInjectedBehaviors(const FGameplayTag& gameplayTag);
   void GetInjectedBehaviors(const FGameplayTag& gameplayTag, TArray<UUtilityBehaviorSet*>& outBehaviors) const;

   DECLARE_MULTICAST_DELEGATE_OneParam(FOnInjectedBehaviorsChanged, UTATUtilityAIBehaviorComponent*)
   void CallAndRegisterInjectionBehaviorChanged(FGameplayTag gameplayTag, FOnInjectedBehaviorsChanged::FDelegate&& delegateToBind);
protected:
   // from AActor
   virtual void BeginPlay() override;

   // from UUtilityAIComponent
   virtual bool _HasCachedPotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) const override;
   virtual void _CachePotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) override;
   virtual const TArray<FUtilityStateTarget>& _GetPotentialTargetsFor(EUtilityStateTargeting targeting, const FGameplayTag& targetingGroup) const override;
   virtual void _ResetPotentialTargetCache() override;
   void _CacheAlwaysKnownSmartObjects();

   // The extents (radii dimensions) of the box surrounding the player to find smart objects
   UPROPERTY(EditDefaultsOnly, Category = "EQS|Action Nodes")
   FVector SmartObjectSearchBoxExtent = FVector(1000.f, 1000.f, 500.f);
   UPROPERTY(EditDefaultsOnly, Category = "EQS|Action Nodes", meta = (Units = "seconds"))
   float SmartObjectQueryFrequency = 5.0f;
   
   // smart object query
   void _OnRunSmartObjectQuery();
   float _GenerateSmartObjectQueryFrequency() const;

   TMap<FGameplayTag, FOnInjectedBehaviorsChanged> InjectionDelegateMap;

   UPROPERTY(Transient)
   TArray<FInjectedBehaviorEntry> InjectedBehaviorEntries;

private:
   UPROPERTY(Transient)
   ATATAIController* _tatAIController = nullptr;
   
   UPROPERTY(Transient)
   FPotentialTargetsCache _potentialTargetsCache;
   
   // smart objects query timer handle
   FTimerHandle _smartObjectQueryTimerHandle;

   // smart objects that are always known about
   TArray<FSmartObjectRequestResult> _alwaysKnownSmartObjects;
};
