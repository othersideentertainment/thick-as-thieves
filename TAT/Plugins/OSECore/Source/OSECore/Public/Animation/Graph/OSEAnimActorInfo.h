// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE4
#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"

// OSE
#include "Character/OSECharacterMovement.h"
#include "Traversal/TraversalInterface.h"
#include "Character/OSECharacterRotation.h"
#include "OSEAnimActorInfo.generated.h"


//--------------------------------------------------------------------------------------------------
/// Thread-safe actor info generated for consumption by the animation instance.
/// This info is expected to be used to generate data that can be used by animation blueprints
/// or animation nodes.
/// 
/// NOTE: Make sure data types are POD types so that it can be utilized in worker threads.
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSEAnimActorInfo
{
   GENERATED_BODY()

public:

   FOSEAnimActorInfo()
      : IsAffectedByDistanceConstraint(0)
      , IsLocallyControlled(0)
   { }

   // Time / interpolation data
   UPROPERTY(EditAnywhere, BlueprintReadOnly) float DeltaT        = 0.0f;
   UPROPERTY(EditAnywhere, BlueprintReadOnly) float OneOverDeltaT = 0.0f;
   UPROPERTY(EditAnywhere, BlueprintReadOnly) float InterpSpeed   = 0.0f;

   // Component data (skeletal mesh component)
   UPROPERTY(EditAnywhere, BlueprintReadOnly) FTransform ComponentTransform = FTransform::Identity;

   // Actor data
   UPROPERTY(EditAnywhere, BlueprintReadOnly) FTransform ActorTransform    = FTransform::Identity;
   UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector    ActorVelocity     = FVector::ZeroVector;
   UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector    ActorEyesLocation = FVector::ZeroVector;
   UPROPERTY(EditAnywhere, BlueprintReadOnly) FRotator   ActorEyesRotation = FRotator::ZeroRotator;

   //Environmental Data
   UPROPERTY(EditAnywhere, BlueprintReadOnly) FFindFloorResult           CurrentFloor;
   UPROPERTY(EditAnywhere, BlueprintReadOnly) FFindWallResult            CurrentWall;
   UPROPERTY(EditAnywhere, BlueprintReadOnly) float                      CurrentFloorDistance = 0.0f;
   UPROPERTY(EditAnywhere, BlueprintReadOnly) float                      TimeSinceLastImpact  = 0.0f;
   UPROPERTY(EditAnywhere, BlueprintReadOnly) bool                       IsFacingWall = false;
   UPROPERTY(EditAnywhere, BlueprintReadOnly) FRotator                   WallAngle = FRotator::ZeroRotator;

   // Movement data
   UPROPERTY(EditAnywhere, BlueprintReadOnly) TEnumAsByte<EMovementMode> MovementMode       = EMovementMode::MOVE_None;
   UPROPERTY(EditAnywhere, BlueprintReadOnly) uint8                      CustomMovementMode = 0;
   UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector                    Acceleration       = FVector::ZeroVector;

   // Traversal data
   UPROPERTY(EditAnywhere, BlueprintReadOnly) FOSETraversalState TraversalState;

   // Turn-in-place data
   UPROPERTY(EditAnywhere, BlueprintReadOnly) FOSETurnInPlaceState TurnInPlaceState;

   // Various state flags
   UPROPERTY(EditAnywhere, BlueprintReadOnly) uint8 IsAffectedByDistanceConstraint : 1;
   UPROPERTY(EditAnywhere, BlueprintReadOnly) uint8 IsLocallyControlled : 1;

   // The resolved pawn (may be null)
   UPROPERTY(Transient, VisibleAnywhere) TWeakObjectPtr<const APawn> PawnOwner = nullptr;

public:

   bool Build(const class UAnimInstance* animInstance, float deltaSeconds, float interpolationSpeed);

   bool IsValid() const { return PawnOwner.IsValid(); }

   const class IGameplayTagAssetInterface* GetGameplayTagInterface() const;
};
