// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// tat
#include "TATMovingBreakable.h"
#include "GameFramework/TATInterpolatedMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMovingBreakable)

ATATMovingBreakable::ATATMovingBreakable()
{
   _movementRoot = CreateDefaultSubobject<USceneComponent>("Movement Root");
   RootComponent = _movementRoot;
   _visualRoot = CreateDefaultSubobject<USceneComponent>("Visual Root");
   _visualRoot->SetupAttachment(_movementRoot);
   _movementComponent = CreateDefaultSubobject<UTATInterpolatedMovementComponent>("MovementComponent");
}

void ATATMovingBreakable::BeginPlay()
{
   Super::BeginPlay();

   _movementComponent->SetInterpolatedComponent(_visualRoot);
}

void ATATMovingBreakable::PostNetReceiveLocationAndRotation()
{
   if (_movementComponent && _movementComponent->UpdatedComponent && _movementComponent->GetInterpMovement())
   {
      const FRepMovement& constRepMovement = GetReplicatedMovement();
      const FVector newLocation = FRepMovement::RebaseOntoLocalOrigin(constRepMovement.Location, this);
      _movementComponent->MoveInterpolationTarget(newLocation, constRepMovement.Rotation);
   }
   else
   {
      Super::PostNetReceiveLocationAndRotation();
   }
}
