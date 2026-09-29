// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Tools/WorldActors/TATToolWorldActor_Base.h"

// ue5
#include "GameplayTagContainer.h"

#include "TATToolWorldActor_MotionSensor.generated.h"

class UOSEShapeCollisionTrackerComponent;

UCLASS()
class TAT_API ATATToolWorldActor_MotionSensor : public ATATToolWorldActor_Base
{
   GENERATED_BODY()
public:
   ATATToolWorldActor_MotionSensor();

   // From UObject
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   // from AActor
   virtual void BeginPlay() override;

   /// Gameplay tags required to be present on any actors for us to track them
   UPROPERTY(EditDefaultsOnly)
   FGameplayTagContainer RequiredTargetTags;

   /// Gameplay tags required to NOT be present on any actors for us to track them
   UPROPERTY(EditDefaultsOnly)
   FGameplayTagContainer BlockedTargetTags;

   /// Get the distance of the closest actor to us
   /// Returns true if a valid actor is in range
   UFUNCTION(BlueprintPure)
   bool GetClosestActorDistance(float& distance) const;

protected:
   UPROPERTY(EditDefaultsOnly)
   UOSEShapeCollisionTrackerComponent* _collisionTrackerComponent = nullptr;

   /// An event that fires if we start detecting any actors
   UFUNCTION(BlueprintImplementableEvent)
   void OnActorsStartDetected();

   /// An event that fires once we no longer detect any actors
   UFUNCTION(BlueprintImplementableEvent)
   void OnActorsNoLongerDetected();

   /// For locally controlled actors, an event that fires whenever a relevant actor enters the area
   /// Meant to be used to fire a toast in BP
   UFUNCTION(BlueprintImplementableEvent)
   void OnRelevantActorDetectedOnLocalClient();

private:
   bool _ShouldTrackActor(const AActor* actor) const;

   UFUNCTION()
   void _OnActorEnteredShape(AActor* actor);

   UFUNCTION()
   void _OnActorExitedShape(AActor* actor);

   UFUNCTION()
   void _OnRep_NumActorEntrancesToArea(int32 oldNumActorEntrancesToArea);

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_NumActorEntrancesToArea)
   int32 _numActorEntrancesToArea = 0;

   UPROPERTY(Transient)
   TArray<TWeakObjectPtr<AActor>> _validActorsInArea;
};
