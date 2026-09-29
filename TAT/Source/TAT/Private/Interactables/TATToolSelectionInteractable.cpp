// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/TATToolSelectionInteractable.h"

// tat
#include "Interactables/TATInteractHighlightUtils.h"
#include "Tools/TATToolComponent.h"

// ose
#include "Items/ToolSetInterface.h"
#include "Items/ToolSetSystemInterface.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToolSelectionInteractable)

ATATToolSelectionInteractable::ATATToolSelectionInteractable()
{
   PrimaryActorTick.bCanEverTick = false;
}

bool ATATToolSelectionInteractable::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   return interactingCharacter->Implements<UToolSetSystemInterface>();
}

void ATATToolSelectionInteractable::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt)
{
   if (_PlayerHasTool(interactingCharacter))
   {
      outPrompt.HoldAction = _GetCachedFormattedRemoveToolPrompt();
   }
   else if (_PlayerHasRoomForMoreTools(interactingCharacter))
   {
      outPrompt.PressAction = _GetCachedFormattedAddToolPrompt();
   }
   else
   {
      outPrompt.ErrorMessage = NoMoreRoomPrompt;
   }
}

FInteractStartResult ATATToolSelectionInteractable::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   if (_PlayerHasTool(interactingCharacter))
   {
      FInteractStartResult startResult = FInteractStartResult::Wait(InteractHoldDuration);
      startResult.HoldAnimationTag = RemoveToolAnimationTag;
      return startResult;
   }
   else if (_PlayerHasRoomForMoreTools(interactingCharacter))
   {
      if (HasAuthority())
      {
         if (TScriptInterface<IToolSetInterface> toolSetInterface = IToolSetInterface::GetToolSetFromActor(interactingCharacter))
         {
            toolSetInterface->AuthorityAddToolClass(ToolClass);
         }
      }

      FInteractStartResult startResult;
      startResult.InstantAnimationTag = AddToolAnimationTag;
      return startResult;
   }
   else
   {
      return FInteractStartResult();
   }
}

bool ATATToolSelectionInteractable::EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context)
{
   if (HasAuthority())
   {
      if (context.IsComplete())
      {
         if (TScriptInterface<IToolSetInterface> toolSetInterface = IToolSetInterface::GetToolSetFromActor(interactingCharacter))
         {
            toolSetInterface->AuthorityRemoveToolsOfClass(ToolClass);
         }
         return true;
      }
   }

   return false;
}

const FText& ATATToolSelectionInteractable::_GetCachedFormattedRemoveToolPrompt()
{
   _MaybeCacheFormattedPrompts();

   return _cachedFormattedRemoveToolPrompt;
}

const FText& ATATToolSelectionInteractable::_GetCachedFormattedAddToolPrompt()
{
   _MaybeCacheFormattedPrompts();

   return _cachedFormattedAddToolPrompt;
}

void ATATToolSelectionInteractable::_MaybeCacheFormattedPrompts()
{
   if (!_cachedPromptsSet && ToolClass)
   {
      UToolComponent* toolCDO = ToolClass->GetDefaultObject<UToolComponent>();
      const FText& toolName = toolCDO->GetToolInfo().ToolName;
      _cachedFormattedAddToolPrompt = FText::FormatNamed(AddToolPrompt, TEXT("ToolName"), toolName);
      _cachedFormattedRemoveToolPrompt = FText::FormatNamed(RemoveToolPrompt, TEXT("ToolName"), toolName);

      _cachedPromptsSet = true;
   }
}

bool ATATToolSelectionInteractable::_PlayerHasTool(ACharacter* character) const
{
   if (TScriptInterface<IToolSetInterface> toolSetInterface = IToolSetInterface::GetToolSetFromActor(character))
   {
      return toolSetInterface->HasToolClass(ToolClass);
   }
   else
   {
      return false;
   }
}

bool ATATToolSelectionInteractable::_PlayerHasRoomForMoreTools(ACharacter* character) const
{
   if (TScriptInterface<IToolSetInterface> toolSetInterface = IToolSetInterface::GetToolSetFromActor(character))
   {
      return toolSetInterface->GetNumTools() < MaxToolCount;
   }
   else
   {
      return false;
   }
}

