// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/Graph/OSEAnimActorInfo.h"

// UE4
#include "Animation/AnimInstance.h"
#include "GameplayTagAssetInterface.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"

// OSE
#include "Character/OSECharacterMovement.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAnimActorInfo)


namespace OSE
{
   namespace AnimUtils
   {
      static const UAnimInstance* ResolveActorAnimInstance(const UAnimInstance* animInstance)
      {
         if (animInstance != nullptr)
         {
            if (const USkeletalMeshComponent* skelMeshComponent = animInstance->GetSkelMeshComponent())
            {
               // If we're using a master pose component, then use that as the mesh
               if (auto leaderPoseComponent = Cast<USkeletalMeshComponent>(skelMeshComponent->LeaderPoseComponent.Get()))
               {
                  skelMeshComponent = leaderPoseComponent;
               }

               // Try to use an attached parent, if we have one
               if (auto parentComponent = Cast<USkeletalMeshComponent>(skelMeshComponent->GetAttachParent()))
               {
                  skelMeshComponent = parentComponent;
               }

               // Now use the animation instance associated with the mesh
               if (const UAnimInstance* mainInstance = skelMeshComponent->GetAnimInstance())
               {
                  return mainInstance;
               }
            }
         }

         return animInstance;
      }

      static const IGameplayTagAssetInterface* ResolveGameplayTagInterface(const AActor* actor)
      {
         if (actor != nullptr)
         {
            // First try to cast directly
            if (auto tagInterface = Cast<IGameplayTagAssetInterface>(actor))
            {
               return tagInterface;
            }

            // Check for ability system component
            if (auto asi = Cast<IAbilitySystemInterface>(actor))
            {
               if (const UAbilitySystemComponent* asc = asi->GetAbilitySystemComponent())
               {
                  if (auto tagInterface = Cast<IGameplayTagAssetInterface>(asc))
                  {
                     return tagInterface;
                  }
               }
            }
         }

         return nullptr;
      }

      static const AActor* GetAttachPawnOrSelf(const AActor* actor)
      {
         if (actor != nullptr)
         {
            // If our attached parent actor is a pawn, return it.
            // Otherwise, return our actor.
            const AActor* attachParentActor = actor->GetAttachParentActor();
            if (Cast<APawn>(attachParentActor) != nullptr)
            {
               return attachParentActor;
            }
            else
            {
               return actor;
            }
         }

         return nullptr;
      }
   }
}

bool FOSEAnimActorInfo::Build(const UAnimInstance* animInstance, float deltaSeconds, float interpolationSpeed)
{
   check(IsInGameThread());

   // Time / interpolation data
   {
      // We want to handle the situation where delta seconds may be zero
      OneOverDeltaT = (deltaSeconds > KINDA_SMALL_NUMBER) ? 1.0f / deltaSeconds : 0.0f;
      DeltaT = deltaSeconds;
      InterpSpeed = interpolationSpeed;
   }

   // Resolve the animation instance to use for the actor
   if (const UAnimInstance* actorInstance = OSE::AnimUtils::ResolveActorAnimInstance(animInstance))
   {
      // Component data
      if (const USkeletalMeshComponent* skelMeshComponent = actorInstance->GetSkelMeshComponent())
      {
         ComponentTransform = skelMeshComponent->GetComponentTransform();
      }

      // Actor data
      if (const AActor* owningActor = actorInstance->GetOwningActor())
      {
         // For certain actor info, we'll use our attached parent if it's a valid pawn
         const AActor* parentOrSelfActor = OSE::AnimUtils::GetAttachPawnOrSelf(owningActor);
         check(parentOrSelfActor != nullptr);

         ActorTransform = parentOrSelfActor->GetActorTransform();
         ActorVelocity = parentOrSelfActor->GetVelocity();
         parentOrSelfActor->GetActorEyesViewPoint(ActorEyesLocation, ActorEyesRotation);

         // Pawn data
         if (auto owningPawn = Cast<APawn>(owningActor))
         {
            // Movement data
            if (auto characterMovement = Cast<UCharacterMovementComponent>(owningPawn->GetMovementComponent()))
            {
               CurrentFloor = characterMovement->CurrentFloor;
               MovementMode = characterMovement->MovementMode;
               CustomMovementMode = characterMovement->CustomMovementMode;
               Acceleration = characterMovement->GetCurrentAcceleration();

               // OSE movement data
               if (auto oseMovement = Cast<UOSECharacterMovement>(characterMovement))
               {
                  CurrentWall = oseMovement->CurrentWall;
                  CurrentFloorDistance = oseMovement->GetFallingFloorDistance();
                  TimeSinceLastImpact = oseMovement->GetTimeSinceLastSlip();
                  IsAffectedByDistanceConstraint = oseMovement->GetAffectedByDistanceConstraint();
                  IsFacingWall = oseMovement->IsVectorWithinWallMovementBounds(ActorEyesRotation.Vector());
                  WallAngle = oseMovement->GetWallAngleNormalRotation();

                  // Turn-in-place data
                  TurnInPlaceState = oseMovement->GetTurnInPlaceState();
               }
            }

            // Traversal data
            if (auto traversalInterface = Cast<ITraversalInterface>(owningPawn))
            {
               TraversalState = traversalInterface->GetTraversalState();
            }

            // Net roles and authority
            IsLocallyControlled = owningPawn->IsLocallyControlled();

            // Success only when this is a pawn; emulates the TryGetPawnOwner behavior and
            // ensures that previewing in the editor allows manual editing of instance data
            PawnOwner = owningPawn;
            return true;
         }
      }
   }

   return false;
}

const IGameplayTagAssetInterface* FOSEAnimActorInfo::GetGameplayTagInterface() const
{
   return OSE::AnimUtils::ResolveGameplayTagInterface(PawnOwner.Get());
}

