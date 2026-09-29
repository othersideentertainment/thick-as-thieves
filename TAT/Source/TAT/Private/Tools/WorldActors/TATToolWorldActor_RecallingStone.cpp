// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/WorldActors/TATToolWorldActor_RecallingStone.h"

// tat
#include "GameFramework/SafeRoom/TATSafeRoom.h"
#include "Player/TATPlayerState.h"
#include "Tools/TATRecallingStoneToolComponent.h"

// ose
#include "Items/ToolSetInterface.h"
#include "Items/ToolSetSystemInterface.h"

// ue5
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToolWorldActor_RecallingStone)

ATATToolWorldActor_RecallingStone::ATATToolWorldActor_RecallingStone() 
{
}

void ATATToolWorldActor_RecallingStone::BeginPlay()
{
   Super::BeginPlay();

   if (HasAuthority())
   {
      if (UTATRecallingStoneToolComponent* recallingStoneTool = _GetRecallingStoneTool())
      {
         recallingStoneTool->AuthoritySetSpawnedTeleportStone(this);

         // This direction is used when resolving overlaps if we are trying to teleport somewhere w/o enough room
         // We want to make sure they're resolved by moving away from any walls it's against,
         // so we have it facing towards our instigator when we're placed (note that FTATTeleportContext::TryLookbackSweep will subtract this vector),
         // the idea being that that is the direction towards an open area
         // We may want a more robust teleport location detection, but this should at least avoid us going through walls
         // NB: This is computed on the server and replicated as an initial value so that the client and server agree on the final teleport location
         _teleportFacingDirection = (GetActorLocation() - GetInstigator()->GetActorLocation()).GetSafeNormal2D();
      }
   }
}

void ATATToolWorldActor_RecallingStone::EndPlay(EEndPlayReason::Type reason)
{
   if (HasAuthority())
   {
      // N.B. Only one recalling stone allowed at a time
      if (UTATRecallingStoneToolComponent* recallingStoneTool = _GetRecallingStoneTool())
      {
         recallingStoneTool->AuthoritySetSpawnedTeleportStone(nullptr);
      }
   }

   Super::EndPlay(reason);
}

void ATATToolWorldActor_RecallingStone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME_CONDITION(ATATToolWorldActor_RecallingStone, _teleportFacingDirection, COND_InitialOnly);
}

bool ATATToolWorldActor_RecallingStone::GetTeleportDestination(FVector& outLocation, FRotator& outRotation) const
{
   const ACharacter* instigatorCharacter = GetInstigator<const ACharacter>();

   // Start the teleport query from our position, offset by half the character's height
   // Otherwise, the query assumes we're trying to teleport into the ground and can get confused
   FTATTeleportTargetParams teleportParams = {};
   teleportParams.MarkerLocation = GetActorLocation() + FVector(0.0f, 0.0f, instigatorCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
   teleportParams.MarkerFacing = _teleportFacingDirection;

   // Take our rotation, and only keep the yaw component
   FRotator rotation = GetActorRotation();
   rotation.Roll = 0.0f;
   rotation.Pitch = 0.0f;
   outRotation = rotation;

   return UTATTeleportUtilities::CalculateTeleportLocation(instigatorCharacter, teleportParams, TeleportSettings, outLocation);
}

void ATATToolWorldActor_RecallingStone::AuthorityOnSurroundingSafeRoomClaimed(ATATSafeRoom* safeRoom)
{
   check(HasAuthority());

   bool safeRoomClaimedByOwningPlayer = false;
   if (APawn* pawnOwner = GetInstigator<APawn>())
   {
      if (pawnOwner->GetPlayerState() == safeRoom->GetOwningPlayer())
      {
         // In this case: the recalling stone was placed by the player claiming the safe room
         // This is okay, so leave the stone placed
         safeRoomClaimedByOwningPlayer = true;
      }
   }

   // In any other case, the recalling stone should not stay since it's now owned by another player
   // Destroy ourselves, which should also unbind us from the player who spawned us, (see AuthoritySetSpawnedTeleportStone call in EndPlay)
   if (!safeRoomClaimedByOwningPlayer)
   {
      Destroy();
   }
}

UTATRecallingStoneToolComponent* ATATToolWorldActor_RecallingStone::_GetRecallingStoneTool() const
{
   if (IToolSetSystemInterface* toolSetSystem = Cast<IToolSetSystemInterface>(GetInstigator()))
   {
      if (TScriptInterface<IToolSetInterface> toolSetInterface = toolSetSystem->GetToolSetInterface())
      {
         // TODO: Update if there's a get-tool-by-class option
         for (int32 i = 0; i < toolSetInterface->GetNumTools(); i++)
         {
            if (auto* recallingStoneTool = Cast<UTATRecallingStoneToolComponent>(toolSetInterface->GetToolAtIndex(i)))
            {
               return recallingStoneTool;
            }
         }
      }
   }
   
   return nullptr;
}

