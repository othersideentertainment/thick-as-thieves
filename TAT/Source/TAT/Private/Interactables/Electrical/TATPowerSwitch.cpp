// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/Electrical/TATPowerSwitch.h"

// tat
#include "Interactables/Electrical/TATPowerSource.h"
#include "Interactables/TATInteractHighlightUtils.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPowerSwitch)
DEFINE_LOG_CATEGORY_STATIC(LogTATPowerSwitch, Log, All);

#if WITH_EDITOR
EDataValidationResult UTATPowerSwitch::IsDataValid(FDataValidationContext& context) const
{
   const EDataValidationResult result = Super::IsDataValid(context);

   if (!_switchUsedGameplayCue.IsValid())
   {
      context.AddError(FText::FromString(TEXT("Unassigned _switchUsedGameplayCue! This is necessary to produce the replicated callback to BroadcastUsed() so the OnUsed hook fires")));
   }
   return context.GetNumErrors() > 0 ? EDataValidationResult::Invalid : result;
}
#endif // WITH_EDITOR

bool UTATPowerSwitch::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   return true;
}

void UTATPowerSwitch::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   if (ATATPowerSource* powerSource = GetPowerSource())
   {
      if (_usePowerSourcePrompts)
      {
         IInteractableInterface::Execute_GetInteractPrompt(powerSource, interactingCharacter, prompt);
      }
      else
      {
         prompt.PressAction = powerSource->IsSwitchedOn() ? _turnOffPrompt : _turnOnPrompt;
      }
   }
}

FInteractStartResult UTATPowerSwitch::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   FInteractStartResult result;
   if (ATATPowerSource* powerSource = GetPowerSource())
   {
      powerSource->Toggle();
   }

   if (_switchUsedGameplayCue.IsValid())
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(interactingCharacter))
      {
         // Execute gameplay cue on character, passing ourselves along so we can replicate a call to BroadcastToggled()
         FGameplayCueParameters params;
         params.SourceObject = this;
         asc->ExecuteGameplayCue(_switchUsedGameplayCue, params); 
      }
   }
   else
   {
      UE_LOG(LogTATPowerSwitch, Error, TEXT("[%s] Missing _switchUsedGameplayCue! Could not execute cue to produce OnUsed callback"), *GetName());
   }

   result.InstantAnimationTag = _interactAnimationTag;
   return result;
}

void UTATPowerSwitch::ShowHighlight_Implementation(bool showHighlight)
{
   UTATInteractHighlightUtils::HighlightInteractMeshes(GetOwner(), showHighlight);
}

ATATPowerSource* UTATPowerSwitch::GetPowerSource() const
{
   if (ATATPowerSource* powerSource = _powerSource.Get())
   {
      return powerSource;
   }
   
   UE_LOG(LogTATPowerSwitch, Warning, TEXT("[%s] GetPowerSource() called for switch missing power source!"), *GetName());
   return nullptr;
}
