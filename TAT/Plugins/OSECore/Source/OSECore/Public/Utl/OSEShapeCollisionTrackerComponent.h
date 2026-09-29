// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Components/ActorComponent.h"

#include "OSEShapeCollisionTrackerComponent.generated.h"

class UShapeComponent;

USTRUCT()
struct OSECORE_API FOSEShapeCollisionTrackerOverlapData
{
   GENERATED_BODY()

public:
   FOSEShapeCollisionTrackerOverlapData() {}
   FOSEShapeCollisionTrackerOverlapData(const float overlapDuration, const int overlapCount) : OverlapDuration(overlapDuration), OverlapCount(overlapCount) {}

   // Amount of time actor has been overlapping with at least 1 of our shape components
   float OverlapDuration;

   // Number of shape components actor is overlapping with
   int32 OverlapCount;
};

/**
* Component that keeps track of overlap events for a collection of UShapeComponents.
*/
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class OSECORE_API UOSEShapeCollisionTrackerComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UOSEShapeCollisionTrackerComponent();
   virtual void InitializeComponent() override;
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

   // if true, uses the first shape component found in the actor we're attached to
   // if false, call SetShapeComponent to set the specific shape component we're listening to
   UPROPERTY(EditDefaultsOnly, Category = "Shape Collision Tracker")
   bool AutomaticallyFindShapeComponent = true;

   // time the players need to be inside of the shape to receive the event.  ideally this is not zero,
   // so we don't run into scenarios where players enter/exit the shape each frame because they're on the edge of it 
   UPROPERTY(EditDefaultsOnly, Category = "Shape Collision Tracker")
   float RequiredTimeInsideOfShape = 0.2f;

   // in general we only want to run these sorts of things on the server, which is where we can apply effects like damage with authority,
   // but maybe there are some use-cases for using this in a locally simulated manner, and if so, turn this off
   UPROPERTY(EditDefaultsOnly, Category = "Shape Collision Tracker")
   bool OnlyRunOnAuthority = true;

   // should we ignore overlaps triggered by the owning actor of this component?
   UPROPERTY(EditDefaultsOnly, Category = "Shape Collision Tracker")
   bool IgnoreOwner = true;

   // Unbinds from existing shape components, clearing their references along with the overlap state
   UFUNCTION(BlueprintCallable, Category = "Shape Collision Tracker")
   void ClearShapeTracking();

   // Set a specific array of shape components to use. Clears bindings to existing shape components as well as tracked overlap state
   UFUNCTION(BlueprintCallable, Category = "Shape Collision Tracker")
   void SetShapeComponents(const TArray<UShapeComponent*>& shapeComponents);

   UFUNCTION(BlueprintCallable, Category = "Shape Collision Tracker")
   void SetShapeComponent(UShapeComponent* shapeComponent);

   /// If we're setting shape components manually post-BeginPlay, we may already have overlaps
   /// In that case, call this to make sure that we start tracking those overlaps, or they will be missed
   UFUNCTION(BlueprintCallable, Category = "Shape Collision Tracker")
   void RefreshInitialOverlaps();

   UFUNCTION(BlueprintPure, Category = "Shape Collision Tracker")
   const TArray<UShapeComponent*>& GetShapeComponents() const { return _shapeComponents; }

   // get a list of currently overlapped actors
   UFUNCTION(BlueprintPure, Category = "Shape Collision Tracker")
   TArray<AActor*> GetOverlappedActors() const;

   // the actor has been inside of the shape for "RequiredTimeInsideOfShape" seconds
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorEnteredShape, AActor*, actor);
   UPROPERTY(BlueprintAssignable, Category = "Shape Collision Tracker")
   FOnActorEnteredShape OnActorEnteredShape;

   // an actor that was previously in our shape for "RequiredTimeInsideOfShape" seconds has now left it
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorExistedShape, AActor*, actor);
   UPROPERTY(BlueprintAssignable, Category = "Shape Collision Tracker")
   FOnActorExistedShape OnActorExitedShape;

private:
   // Set a specific array of shape components to use. Clears bindings to existing shape components as well as tracked overlap state
   void _SetShapeComponents(const TArrayView<UShapeComponent* const>& shapeComponents);

   UFUNCTION()
   void _OnShapeBeginOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult);
   UFUNCTION()
   void _OnShapeEndOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex);

   UFUNCTION()
   void _HandleInitialOverlaps();

   void _HandleBeginOverlapWithActor(AActor* otherActor);

   bool _IsEnabled() const;

private:
   UPROPERTY()
   TArray<UShapeComponent*> _shapeComponents;

   // Keeps track of overlap state for any actors currently overlapping with a shape component
   UPROPERTY()
   TMap<AActor*, FOSEShapeCollisionTrackerOverlapData > _actorOverlapState;
};
