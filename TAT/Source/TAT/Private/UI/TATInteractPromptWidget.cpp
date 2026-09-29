// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "UI/TATInteractPromptWidget.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"

// tat
#include "Interactables/TATInteractionTargeterComponent.h"
#include "Player/TATCharacter.h"

// ue5
#include "Abilities/GameplayAbility.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATInteractPromptWidget)

DEFINE_LOG_CATEGORY_STATIC(LogTATInteractPromptWidget, Log, All);

void UTATInteractPromptWidget::NativeConstruct()
{
   ReceiveOnLocalCharacterIsReady = true;
   Super::NativeConstruct();

   // empty the text out on construct
   _OnPromptChanged();
}

void UTATInteractPromptWidget::NativeDestruct()
{
   Super::NativeDestruct();
}

void UTATInteractPromptWidget::RequestAbilityInteractPrompt(UGameplayAbility* ability, bool isPress)
{
   check(ability);

   FAbilityPromptRequest req;
   req.Ability = ability;
   req.IsPress = isPress;

   if (!_requestedAbilityPrompts.Contains(req))
   {
      _requestedAbilityPrompts.Add(req);  
   }

   // refresh the prompt whether or not it's in the list -- if it's not we can assume this is a text refresh quest
   _OnPromptChanged();
}

void UTATInteractPromptWidget::ClearRequestedInteractPrompt(UGameplayAbility* ability, bool isPress)
{
   check(ability);

   FAbilityPromptRequest req;
   req.Ability = ability;
   req.IsPress = isPress;

   _requestedAbilityPrompts.Remove(req);
   _OnPromptChanged();
}

void UTATInteractPromptWidget::RequestAbilityExtraContextualInput(UGameplayAbility* ability, EAbilityInputType inputType, bool isPress)
{
   check(ability);

   if (inputType != EAbilityInputType::AbilityInteract)
   {
      bool alreadyPresent = _requestedAbilityExtraContextualInputs.ContainsByPredicate([&](const FAbilityExtraContextualInputRequest& contextualInputRequest)
      {
         return contextualInputRequest.Ability.Get() == ability
            && contextualInputRequest.InputType == inputType
            && contextualInputRequest.IsPress == isPress;
      });

      if (!alreadyPresent)
      {
         FAbilityExtraContextualInputRequest req;
         req.Ability = ability;
         req.IsPress = isPress;
         req.InputText = UOSEInteractionHelpers::GetPromptForAbility(ability);
         req.InputType = inputType;
         req.ActionTag = UOSEInteractionHelpers::GetPromptActionTagForAbility(ability);

         _requestedAbilityExtraContextualInputs.Add(req);
      }

      _OnExtraContextualInputPromptsChanged();
   }
   else
   {
      RequestAbilityInteractPrompt(ability, isPress);
   }
}

void UTATInteractPromptWidget::ClearAbilityExtraContextualInput(UGameplayAbility* ability, EAbilityInputType inputType, bool isPress)
{
   check(ability);


   if (inputType == EAbilityInputType::AbilityInteract)
   {
      ClearRequestedInteractPrompt(ability, isPress);
      return;
   }

   _requestedAbilityExtraContextualInputs.RemoveAll([&](const FAbilityExtraContextualInputRequest& contextualInputRequest)
   {
      return contextualInputRequest.Ability.Get() == ability
         && contextualInputRequest.InputType == inputType
         && contextualInputRequest.IsPress == isPress;
   });

   _OnExtraContextualInputPromptsChanged();
}

void UTATInteractPromptWidget::_OnLocalCharacterIsReady_Implementation(AOSECharacterBase* character)
{
   if (UTATInteractionTargeterComponent* interactTargetComponent = character->FindComponentByClass<UTATInteractionTargeterComponent>())
   {
      interactTargetComponent->OnPromptChanged.AddUniqueDynamic(this, &UTATInteractPromptWidget::_OnInteractTargeterPromptChanged);

      // initial update in case we already have a prompt!
      _targeterPrompt = interactTargetComponent->GetCurrentPrompt();
      _OnPromptChanged();
   }
   else
   {
      UE_LOG(LogTATInteractPromptWidget, Warning, TEXT("No interact target component on our local character?"));
   }   
}

void UTATInteractPromptWidget::_OnInteractTargeterPromptChanged(const FInteractPrompt& prompt)
{
   _targeterPrompt = prompt;
   _OnPromptChanged();
}

void UTATInteractPromptWidget::_CleanupRequestedPrompts()
{
   // clear up anything that's nullptr due to the weak ability ptr
   for (auto it = _requestedAbilityPrompts.CreateIterator(); it; ++it)
   {
      if (!IsValid(it->Ability.Get()))
         it.RemoveCurrent();
   }

   for (auto it = _requestedAbilityExtraContextualInputs.CreateIterator(); it; ++it)
   {
      if (!IsValid(it->Ability.Get()))
         it.RemoveCurrent();
   }
}

void UTATInteractPromptWidget::_OnPromptChanged()
{
   // cleanup anything stale
   _CleanupRequestedPrompts();

   // resolve which prompt to show based on the targeting prompt + other systems requesting a prompt
   // TODO: Assumes most recent thing on top, which doesn't really imply a priority, but we don't quite need that yet either...?
   if (_requestedAbilityPrompts.Num() > 0)
   {
      const FAbilityPromptRequest& req = _requestedAbilityPrompts[_requestedAbilityPrompts.Num() - 1];
      UGameplayAbility* abilityPtr = req.Ability.Get();
      FText text = UOSEInteractionHelpers::GetPromptForAbility(abilityPtr);

      FInteractPrompt prompt;
      if (req.IsPress)
      {
         prompt.PressAction = text;
         prompt.PressActionTag = UOSEInteractionHelpers::GetPromptActionTagForAbility(abilityPtr);
      }
      else
      {
         prompt.HoldAction = text;
         prompt.HoldActionTag = UOSEInteractionHelpers::GetPromptActionTagForAbility(abilityPtr);
      }
      
      _OnPromptChanged(prompt);
   }
   else if (_targeterPrompt.IsValid())
   {
      _OnPromptChanged(_targeterPrompt);
   }
   else
   {
      _OnPromptChanged(FInteractPrompt());
   }
}

void UTATInteractPromptWidget::_OnExtraContextualInputPromptsChanged()
{
   // cleanup anything stale
   _CleanupRequestedPrompts();

   _OnExtraContextualInputPromptsChanged(_requestedAbilityExtraContextualInputs);
}

