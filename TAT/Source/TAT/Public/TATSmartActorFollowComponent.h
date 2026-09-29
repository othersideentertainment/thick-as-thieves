// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TATSmartActorFollowComponent.generated.h"

// A component that maintains a path from the _sourceActor to the _targetActor using breadcrumbs
// Maintained path is a list of _goodLocations that are all within Line of Sight to each other
// This component is meant to only provide information of where to go next, not move the actor
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TAT_API UTATSmartActorFollowComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   // Sets default values for this component's properties
   UTATSmartActorFollowComponent();

   // Called every frame
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

   UFUNCTION(BlueprintCallable)
   bool SetSourceAndTargetActors(AActor* source, AActor* target);

   // Returns the next location toward the _targetActor that the _sourceActor has Line of Sight of
   // (Either _goodLocations[0] or _targetActor)
   UFUNCTION(BlueprintCallable)
   FVector GetNextLocation();

   // Checks to see if the _sourceActor has reached either _goodLocation[0] or the _targetActor
   // If _goodLocation[0] was reached, removes the location from the array and returns false
   // If _targetActor was reached, returns true
   UFUNCTION(BlueprintCallable)
   bool CheckIfReachedTargetAndUpdateGoodLocations();

   // The distance the _sourceActor needs to be from a _goodLocation[i] or the _targetActor to be considered as "arrived"
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   float _reachedTargetDistance = 10.0f;

protected:
   // Called when the game starts
   virtual void BeginPlay() override;

   // Actor that will be following the maintained path
   TWeakObjectPtr<AActor> _sourceActor;
   // The Actor that is effectively the moving destination of the maintained path 
   TWeakObjectPtr<AActor> _targetActor;

   // The last known good location of _targetActor
   // *good* here signifies that the location is reachable through a series of Line of Sight checks (breadcrumbs)
   UPROPERTY(Transient)
   FVector _lastKnownGoodLocation;
   // An array of locations that the _targetActor has moved through, essentially creating breadcrumbs
   // This is the "maintained path" and should result in a chain of locations
   // Each location should have Line of Sight to it's neighboring locations
   // The 0 index location is the closest location to the _sourceActor and should have Line of Sight of the _sourceActor
   // The last index location is the closest location to the _targetActor and should have Line of Sight of the _targetActor
   UPROPERTY(Transient)
   TArray<FVector> _goodLocations;

private:

   FVector _GetTargetLocation() const;
   // If there is no blocking collision in a trace between Source Location and Target Location, then this will return true
   bool _CheckForLineOfSight(const FVector& sourceLocation, const FVector& targetLocation, const FCollisionQueryParams& collisionQueryParams, FVector& outBlockingLocation) const;
};
