// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/Graph/OSEAnimDataLibrary.h"

// OSE
#include "Character/OSECharacterMovement.h"

// UE4
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAnimDataLibrary)


EOSEAnimState UOSEAnimDataFunctionLibrary::GetAnimState(const APawn* pawnOwner)
{
   if (pawnOwner == nullptr)
      return EOSEAnimState::Unknown;

   return GetAnimState(pawnOwner->GetMovementComponent());
}

EOSEAnimState UOSEAnimDataFunctionLibrary::GetAnimState(const UPawnMovementComponent* pawnMovementComponent)
{
   if (auto movementComponent = Cast<UCharacterMovementComponent>(pawnMovementComponent))
   {
      return GetAnimState(movementComponent->MovementMode, movementComponent->CustomMovementMode);
   }

   return EOSEAnimState::Unknown;
}

EOSEAnimState UOSEAnimDataFunctionLibrary::GetAnimState(EMovementMode movementMode, uint8 customMode)
{
   switch (movementMode)
   {
   case EMovementMode::MOVE_Walking:
   case EMovementMode::MOVE_NavWalking:
      return EOSEAnimState::Walking;
      break;

   case EMovementMode::MOVE_Falling:
      return EOSEAnimState::Falling;
      break;

   case EMovementMode::MOVE_Swimming:
      return EOSEAnimState::Swimming;
      break;

   case EMovementMode::MOVE_Flying:
      return EOSEAnimState::Flying;
      break;

   default:
      break;
   }

   switch (OSE::MovementUtils::GetCustomMovementType(movementMode, customMode))
   {
   case ECustomMovementType::Scramble:
      return EOSEAnimState::Scrambling;
      break;

   case ECustomMovementType::Mantle:
      return EOSEAnimState::Mantling;
      break;

   default:
      break;
   }

   return EOSEAnimState::Unknown;
}

bool UOSEAnimDataFunctionLibrary::GetFloorRotation(FRotator& outRotation, const APawn* pawnOwner)
{
   const UPawnMovementComponent* pawnMovement = (pawnOwner != nullptr) ? pawnOwner->GetMovementComponent() : nullptr;
   if (pawnMovement != nullptr && pawnMovement->IsMovingOnGround())
   {
      if (auto characterMovement = Cast<UCharacterMovementComponent>(pawnMovement))
      {
         const FTransform& localToWorld = pawnOwner->GetActorTransform();
         const FFindFloorResult& currentFloor = characterMovement->CurrentFloor;
         return GetFloorRotation(outRotation, currentFloor, localToWorld);
      }
   }

   outRotation = FRotator::ZeroRotator;
   return false;
}

bool UOSEAnimDataFunctionLibrary::GetFloorRotation(FRotator& outRotation, const FFindFloorResult& floorResult, const FTransform& localToWorld)
{
   if (floorResult.IsWalkableFloor())
   {
      outRotation = GetFloorRotation(floorResult.HitResult, localToWorld);
      return true;
   }

   outRotation = FRotator::ZeroRotator;
   return false;
}

FRotator UOSEAnimDataFunctionLibrary::GetFloorRotation(const FHitResult& hitResult, const FTransform& localToWorld)
{
   // Hit results from FFindFloorResult _may_ not be valid blocking hits, as they are
   // updated in the character movement component with a line trace
   if (hitResult.bBlockingHit)
   {
      // Make sure the normal is pointing up, since we assume it came from a floor
      const FVector& axisZ = hitResult.ImpactNormal;
      if (axisZ.Z > 0.0f)
      {
         // Use the Y basis vector of the matrix to derive the X basis vector;
         // this ensures that the pitch will be the most accurate projection
         const FVector axisY = localToWorld.TransformVector(FVector::RightVector);
         return FRotationMatrix::MakeFromZY(axisZ, axisY).Rotator();
      }
   }

   return FRotator::ZeroRotator;
}

TEnumAsByte<EPhysicalSurface> UOSEAnimDataFunctionLibrary::GetPhysicalSurfaceFromWall(const FFindWallResult& wallResult)
{
   if(UPhysicalMaterial* physMat = wallResult.HitResult.PhysMaterial.Get())
   {
      return physMat->SurfaceType;
   }

   return EPhysicalSurface::SurfaceType_Default;
}

