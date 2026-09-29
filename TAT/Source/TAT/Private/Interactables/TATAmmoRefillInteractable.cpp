// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/TATAmmoRefillInteractable.h"

// tat
#include "Interactables/TATInteractHighlightUtils.h"
#include "Tools/TATToolSetComponent.h"

// ose
#include "Items/ToolSetInterface.h"
#include "Items/ToolSetSystemInterface.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAmmoRefillInteractable)

DEFINE_LOG_CATEGORY_STATIC(LogTATAmmoRefillInteractable, Log, All)

ATATAmmoRefillInteractable::ATATAmmoRefillInteractable()
{
   PrimaryActorTick.bCanEverTick = false;
   _interactHoldDuration = 5.f;
}

bool ATATAmmoRefillInteractable::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   return interactingCharacter->Implements<UToolSetSystemInterface>();
}

void ATATAmmoRefillInteractable::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt)
{
   outPrompt.HoldAction = _interactPrompt;
}

FInteractStartResult ATATAmmoRefillInteractable::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   FInteractStartResult result = FInteractStartResult::Wait(_interactHoldDuration);
   result.HoldAnimationTag = _interactAnimationTag;
   result.HoldActionCues = _holdCues;
   return result;
}

bool ATATAmmoRefillInteractable::EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context)
{
   if (context.IsComplete())
   {
      // actually do the refill on authority
      if (HasAuthority())
      {
         if (TScriptInterface<IToolSetInterface> toolSetInterface = IToolSetInterface::GetToolSetFromActor(interactingCharacter))
         {
            if (UTATToolSetComponent* tatToolSet = Cast<UTATToolSetComponent>(toolSetInterface.GetObject()))
            {
               tatToolSet->AuthorityRefillAmmoForStartingGear();
            }
            else
            {
                UE_LOG(LogTATAmmoRefillInteractable, Error,
                   TEXT("ATATAmmoRefillInteractable expected a UTATToolSetComponent, found '%s'"),
                   *GetNameSafe(toolSetInterface.GetObject()));
            }
         }
      }
   }

   return true;
}

void ATATAmmoRefillInteractable::ShowHighlight_Implementation(bool showHighlight)
{
   // TODO: Just allow this to be done on the bp side...?
   UTATInteractHighlightUtils::HighlightInteractMeshes(this, showHighlight);
}
