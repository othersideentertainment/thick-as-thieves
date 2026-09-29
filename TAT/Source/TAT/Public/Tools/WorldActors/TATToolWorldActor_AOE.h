// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Tools/WorldActors/TATToolWorldActor_Base.h"

// ose
#include "Abilities/OSEActorsWithAppliedEffectsSet.h"

// ue5
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"

#include "TATToolWorldActor_AOE.generated.h"

class UGameplayEffect;
class UOSEShapeCollisionTrackerComponent;

UCLASS()
class TAT_API ATATToolWorldActor_AOE : public ATATToolWorldActor_Base
{
   GENERATED_BODY()
public:
   ATATToolWorldActor_AOE();

   // From UObject
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

   // From AActor
   virtual void BeginPlay() override;
   virtual void EndPlay(EEndPlayReason::Type reason) override;

   UFUNCTION(BlueprintImplementableEvent)
   void BP_Authority_OnActorEnterShape(AActor* actor);

   UFUNCTION(BlueprintImplementableEvent)
   void BP_Authority_OnActorExitShape(AActor* actor);

   /// Fires on all clients when we first trigger on an actor
   UFUNCTION(BlueprintImplementableEvent)
   void BP_OnFirstActivated(bool justHappened);

   UPROPERTY(EditDefaultsOnly)
   TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;

   UPROPERTY(EditDefaultsOnly)
   bool IgnoreInstigator = true;

   /// If set to 0, we will immediately be active on spawn.
   /// Otherwise, we will be inactive for N seconds post-spawn to allow for animations or other visuals
   UPROPERTY(EditDefaultsOnly)
   float SecondsAfterSpawnToActivate = 0.0f;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTagQuery TriggerTargetCriteria;

protected:
   
   // From ATATToolWorldActor_Base
   virtual bool _HasBeenActivated() const override { return _triggeredTimestamp >= 0.0f; }
   
   UPROPERTY(EditDefaultsOnly)
   UOSEShapeCollisionTrackerComponent* _collisionTrackerComponent = nullptr;

private:
   UFUNCTION()
   void _AuthorityActivateAOE();

   UFUNCTION()
   void _AuthorityOnActorEnterShape(AActor* actor);

   UFUNCTION()
   void _AuthorityOnActorExitShape(AActor* actor);

   UFUNCTION()
   void _OnRep_TriggeredTimestamp();

   UPROPERTY(Transient)
   FOSEActorsWithAppliedEffectsSet _actorsWithEffectApplied;

   /// This is set by the server when we first trigger on any actor, and is replicated
   /// so clients can adjust visual state, etc.
   UPROPERTY(Transient, ReplicatedUsing = _OnRep_TriggeredTimestamp)
   float _triggeredTimestamp = -1.0f;
};
