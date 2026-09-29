// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Tools/WorldActors/TATToolWorldActor_Base.h"

// ue5
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"

#include "TATToolWorldActor_Proximity.generated.h"

class UOSEShapeCollisionTrackerComponent;

/// An actor spawned by tools, that triggers some functionality when another actor gets too close
/// Example: the cuckoo clock will attach itself to the first character to come close to it
/// By default ignores the instigator, and can have additional tag constraints specified
UCLASS()
class TAT_API ATATToolWorldActor_Proximity : public ATATToolWorldActor_Base
{
   GENERATED_BODY()
public:
   ATATToolWorldActor_Proximity();

   // From UObject
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

   // From AActor
   virtual void BeginPlay() override;
 
   UFUNCTION(BlueprintImplementableEvent)
   void OnAuthorityTriggeredBy(AActor* actor);

   /// Fires on the client and server
   /// The justHappened flag tells us if this is an event that just triggered (and thus should play transient VFX),
   /// or if it happened a while ago and we're just getting the replication
   UFUNCTION(BlueprintImplementableEvent)
   void OnTriggered(bool justHappened);

   UPROPERTY(EditDefaultsOnly)
   bool IgnoreInstigator = true;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTagQuery TriggerTargetCriteria;

protected:
   // From ATATToolWorldActor_Base
   virtual bool _HasBeenActivated() const override { return _triggeredTimestamp >= 0.0f; }

   UPROPERTY(EditDefaultsOnly)
   UOSEShapeCollisionTrackerComponent* _collisionTrackerComponent = nullptr;

   void _OnAuthorityTriggeredBy(AActor* actor);

   UPROPERTY(EditDefaultsOnly, Category = Stims, meta = (Categories = "AI.Stim.Hearing"))
   FGameplayTag _triggerHearingStim;

private:
   UFUNCTION()
   void _AuthorityOnActorEnterShape(AActor* actor);

   UFUNCTION()
   void _OnRep_TriggeredTimestamp();

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_TriggeredTimestamp)
   float _triggeredTimestamp = -1.0f;
};
