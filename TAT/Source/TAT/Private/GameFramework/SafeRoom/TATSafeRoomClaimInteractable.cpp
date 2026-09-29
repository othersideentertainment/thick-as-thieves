// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "GameFramework/SafeRoom/TATSafeRoomClaimInteractable.h"

// tat
#include "GameFramework/SafeRoom/TATSafeRoom.h"
#include "Player/TATPlayerState.h"

// ue5
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSafeRoomClaimInteractable)

DEFINE_LOG_CATEGORY_STATIC(LogTATSafeRoomClaimInteractable, Log, All)

void ATATSafeRoomClaimInteractable::BeginPlay()
{
   Super::BeginPlay();

   if (ensure(IsValid(_safeRoom)))
   {
      _OnOwnerTypeChanged(_safeRoom->GetOwnerType());
      _safeRoom->OnOwnerTypeChanged.AddUniqueDynamic(this, &ATATSafeRoomClaimInteractable::_OnOwnerTypeChanged);
   }
}

bool ATATSafeRoomClaimInteractable::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   check(IsValid(_safeRoom));

   // Only allow interaction from player
   if (interactingCharacter->GetPlayerState() == nullptr)
   {
      return false;
   }

   // NB: Deliberately not checking for other players in the room so we can show an error prompt instead
   return _safeRoom->GetOwningPlayer() == nullptr;
}

FInteractStartResult ATATSafeRoomClaimInteractable::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   FInteractStartResult result;

   result.bWaitForDelay = true;
   result.Delay = HoldDuration;
   result.HoldAnimationTag = HoldAnimationTag;
   result.HoldActionCues = HoldActionCues;

   return result;
}

bool ATATSafeRoomClaimInteractable::EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context)
{
   if (context.IsComplete())
   {
      if (HasAuthority())
      {
         if (ATATPlayerState* interactorPS = interactingCharacter->GetPlayerState<ATATPlayerState>())
         {
            // Try to claim the safe room
            // TODO: If it fails at this stage, we should try to give some feedback to the user
            _safeRoom->AuthorityTryClaim(interactorPS);
         }
         else
         {
            UE_LOG(LogTATSafeRoomClaimInteractable, Warning,
               TEXT("Character '%s' interacting with safe room claim interactable, but does not have the expected player state"),
               *interactingCharacter->GetName());
         }
      }
   }

   return false;
}

void ATATSafeRoomClaimInteractable::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt)
{
   if (_safeRoom->CanBeClaimedBy(interactingCharacter))
   {
      outPrompt.HoldAction = HoldPrompt;
   }
   else
   {
      outPrompt.ErrorMessage = CannotClaimError;
   }
}

void ATATSafeRoomClaimInteractable::_OnOwnerTypeChanged(ETATSafeRoomOwnerType ownerType)
{
   BP_OnOwnerTypeChanged(ownerType);
}

