// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// tat
#include "TATSmartActorFollowComponent.h"

// tat
#include "Tools/TATToolFunctionLibrary.h"

// ose
#include "Interactables/InteractableInterface.h"

// ue
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSmartActorFollowComponent)

DEFINE_LOG_CATEGORY_STATIC(LogSmartActorFollowComponent, Log, All);

// Sets default values for this component's properties
UTATSmartActorFollowComponent::UTATSmartActorFollowComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bAllowTickOnDedicatedServer = true;
   PrimaryComponentTick.bStartWithTickEnabled = false;
}


// Called when the game starts
void UTATSmartActorFollowComponent::BeginPlay()
{
   Super::BeginPlay();
}


// Called every frame
void UTATSmartActorFollowComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   if (_sourceActor != nullptr && _targetActor != nullptr)
   {
      FVector sourceLocation = _sourceActor->GetActorLocation();
      FVector targetLocation = _GetTargetLocation();
      FCollisionQueryParams queryParams(SCENE_QUERY_STAT(TATSmartActorFollowComponent_Trace));
      queryParams.AddIgnoredActor(_sourceActor.Get());
      queryParams.AddIgnoredActor(_targetActor.Get());
      queryParams.bIgnoreTouches = true;
      FVector blockingLocation;
      
      // If the source actor does not have Line of Sight of the target actor...
      if (!_CheckForLineOfSight(sourceLocation, targetLocation, queryParams, blockingLocation))
      {
         // Then first check if our last good location does have Line of Sight
         int numGoodLocations = _goodLocations.Num();
         bool hasGoodLocations = numGoodLocations > 0;
         FVector lastGoodLocation = hasGoodLocations ? _goodLocations[numGoodLocations - 1] : sourceLocation;
         // Skip this trace if our _goodLocations Array is empty (hasGoodLocations == true), we don't need to check from _sourceActor to _targetActor twice
         if (!(hasGoodLocations && _CheckForLineOfSight(lastGoodLocation, targetLocation, queryParams, blockingLocation)))
         {
            // If we have no Line of Sight, then re-check Line of Sight to the _lastKnownGoodLocation (we're checking to see if a door or window was closed on us)
            if (_CheckForLineOfSight(lastGoodLocation, _lastKnownGoodLocation, queryParams, blockingLocation))
            {
               // If we still have Line of Sight here, then add the _lastKnownGoodLocation to our list of _goodLocations if it's not the same as our last goodLocation
               if (_lastKnownGoodLocation != lastGoodLocation)
               {
                  _goodLocations.Add(_lastKnownGoodLocation);
               }

               // Then check one more time to see if the _lastKnownGoodLocation has Line of Sight to the current targetLocation
               if (_CheckForLineOfSight(_lastKnownGoodLocation, targetLocation, queryParams, blockingLocation))
               {
                  // If there is Line of Sight, update _lastKnownGoodLocation
                  _lastKnownGoodLocation = targetLocation;
               }
               // If there is no Line of Sight, keep the _lastKnownGoodLocation as the final location for where to move
            }
            else
            {
               // Otherwise, if a door or window has in fact been shut on us, update the _lastKnownGoodLocation
               _lastKnownGoodLocation = blockingLocation;
            }
         }
         else
         {
            // If our last goodLocation does have Line of Sight, just update our _lastKnownGoodLocation
            _lastKnownGoodLocation = targetLocation;
         }
      }
      else
      {
         // We land here if our _sourceLocation has Line of Sight to our _targetLocation
         // In which case, we update our _lastKnownGoodLocation and clear out the _goodLocations array since we can just go straight to the target
         _lastKnownGoodLocation = targetLocation;
         _goodLocations.Empty();
      }
   }
}

bool UTATSmartActorFollowComponent::SetSourceAndTargetActors(AActor* source, AActor* target)
{
   check(source);
   check(target);
   if (!source)
   {
      UE_LOG(LogSmartActorFollowComponent, Warning, TEXT("Invalid source actor passed into FollowBreadCrumbsComponent.  Component will not work!"));
      return false;
   }

   if (!target)
   {
      UE_LOG(LogSmartActorFollowComponent, Warning, TEXT("Invalid target actor passed into FollowBreadCrumbsComponent.  Component will not work!"));
      return false;
   }

   FVector sourceLocation = source->GetActorLocation();
   _targetActor = target;
   FVector targetLocation = _GetTargetLocation();
   FCollisionQueryParams queryParams(SCENE_QUERY_STAT(TATSmartActorFollowComponent_InitTrace));
   queryParams.AddIgnoredActor(source);
   queryParams.bIgnoreTouches = true;
   FVector blockingLocation;

   if (_CheckForLineOfSight(sourceLocation, targetLocation, queryParams, blockingLocation))
   {
      _sourceActor = source;
      _targetActor = target;
      _lastKnownGoodLocation = _GetTargetLocation();
      if (GetOwnerRole() == ROLE_Authority)
      {
         SetComponentTickEnabled(true);
      }
      return true;
   }

   UE_LOG(LogSmartActorFollowComponent, Warning, TEXT("FollowBreadCrumbsComponent: There is no line of sight between the source actor and target actor.  This component needs an initial line of sight to function."));
   _targetActor = nullptr;
   return false;
}

FVector UTATSmartActorFollowComponent::GetNextLocation()
{
   if (_goodLocations.Num() > 0)
   {
      return _goodLocations[0];
   }

   return _lastKnownGoodLocation;
}

bool UTATSmartActorFollowComponent::CheckIfReachedTargetAndUpdateGoodLocations()
{
   check(_sourceActor.Get());
   
   // It's possible that our target was killed or otherwise despawned at some point, so let's add a check here for that and early out
   if (!_targetActor.IsValid())
   {
      // Return true to say we no longer have a path to follow, handle null target wherever this function is called
      return true;
   }
   
   if (_goodLocations.Num() > 0)
   {
      if (FVector::Dist(_sourceActor->GetActorLocation(), _goodLocations[0]) <= _reachedTargetDistance)
      {
         _goodLocations.RemoveAt(0);
         return false;
      }
   }

   if (FVector::Dist(_sourceActor->GetActorLocation(), _GetTargetLocation()) <= _reachedTargetDistance)
   {
      return true;
   }

   return false;
}

FVector UTATSmartActorFollowComponent::_GetTargetLocation() const
{
   AActor* targetActor = _targetActor.Get();
   check(targetActor != nullptr);

   // If the target actor is a non-character interactable, use the location of the actor's interact mesh component
   if (!targetActor->IsA<ACharacter>() && targetActor->Implements<UInteractableInterface>())
   {
      if (USceneComponent* targetComponent = UTATToolFunctionLibrary::FindFirstInteractableComponentInActor(targetActor))
      {
         return targetComponent->GetComponentLocation();
      }
   }

   FVector targetLocation = targetActor->GetActorLocation();
   FVector outTargetLocationEyesViewPoint;
   FRotator outTargetRotation;
   targetActor->GetActorEyesViewPoint(outTargetLocationEyesViewPoint, outTargetRotation);
   // Only add half the difference from the target origin to the target view point so that we get to around the chest area.
   targetLocation += (outTargetLocationEyesViewPoint - targetLocation) / 2.0f;

   return targetLocation;
}

bool UTATSmartActorFollowComponent::_CheckForLineOfSight(const FVector& sourceLocation, const FVector& targetLocation, const FCollisionQueryParams& collisionQueryParams, FVector& outBlockingLocation) const
{
   UWorld* world = GetOwner()->GetWorld();
   check(world);

   FHitResult Hit;
   if (world->LineTraceSingleByChannel(Hit,sourceLocation, targetLocation, ECollisionChannel::ECC_Visibility, collisionQueryParams))
   {
      if (Hit.GetActor() != _targetActor)
      {
         outBlockingLocation = Hit.Location;
         return false;
      }
   }

   outBlockingLocation = FVector::ZeroVector;
   return true;
}

