// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/TATGameplayAbility_ReceiveHoldAndDoublePress.h"

// ue
#include "AbilitySystemComponent.h"
#include "Misc/DataValidation.h"

// ose
#include "Abilities/Tasks/AbilityTask_WaitInputWithTimeout.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_ReceiveHoldAndDoublePress)

DEFINE_LOG_CATEGORY_STATIC(LogTATGameplayAbility_ReceiveHoldAndDoublePress, Log, All);

UTATGameplayAbility_ReceiveHoldAndDoublePress::UTATGameplayAbility_ReceiveHoldAndDoublePress()
{
   ActivationRequiresPlayerController = true;
}

#if WITH_EDITOR
EDataValidationResult UTATGameplayAbility_ReceiveHoldAndDoublePress::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   if (ReceivePressAndHold && PressHoldThresholdSeconds <= 0.f)
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("Ability %s has ReceivePressAndHold = true, but PressHoldThresholdSeconds is invalid (%f)! Either disable ReceivePressAndHold or increase PressHoldThresholdSeconds")
         , *GetName()
         , PressHoldThresholdSeconds)));
      result = EDataValidationResult::Invalid;
   }

   if (ReceiveDoublePress && DoublePressThresholdSeconds <= 0.f)
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("Ability %s has ReceiveDoublePress = true, but DoublePressThresholdSeconds is invalid (%f)! Either disable ReceiveDoublePress or increase DoublePressThresholdSeconds")
         , *GetName()
         , DoublePressThresholdSeconds)));
      result = EDataValidationResult::Invalid;
   }

   return result;
}
#endif // WITH_EDITOR

void UTATGameplayAbility_ReceiveHoldAndDoublePress::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   Super::ActivateAbility(handle, actorInfo, activationInfo, triggerEventData);

   if (!ReceivePressAndHold && !ReceiveDoublePress)
   {
      HandleSinglePress();
      return;
   }

   // listen for initial press release, or a hold exceeding PressHoldThresholdSeconds
   _waitInputReleaseTask = UAbilityTask_WaitInputWithTimeout::WaitInputReleaseWithTimeout(this, PressHoldThresholdSeconds);
   _waitInputReleaseTask->OnInput.AddDynamic(this, &UTATGameplayAbility_ReceiveHoldAndDoublePress::_OnInitialPressReleased);
   _waitInputReleaseTask->OnTimeOut.AddDynamic(this, &UTATGameplayAbility_ReceiveHoldAndDoublePress::_OnInitialPressTimeOut);
   _waitInputReleaseTask->ReadyForActivation();
}

void UTATGameplayAbility_ReceiveHoldAndDoublePress::_OnInitialPressReleased(float timeHeld, bool bTimedOut)
{
   const bool useClientTime = true;
   const bool testAlreadyPressed = false;

   // listen for second press within DoublePressThresholdSeconds
   if (ReceiveDoublePress)
   {
      _waitInputPressTask = UAbilityTask_WaitInputWithTimeout::WaitInputPressWithTimeout(this, DoublePressThresholdSeconds, testAlreadyPressed, useClientTime);
      _waitInputPressTask->OnInput.AddDynamic(this, &UTATGameplayAbility_ReceiveHoldAndDoublePress::_OnDoublePress);
      _waitInputPressTask->OnTimeOut.AddDynamic(this, &UTATGameplayAbility_ReceiveHoldAndDoublePress::_OnDoublePressTimeOut);
      _waitInputPressTask->ReadyForActivation();
   }
   else
   {
      HandleSinglePress();
   }
}

void UTATGameplayAbility_ReceiveHoldAndDoublePress::_OnDoublePress(float timeHeld, bool bTimedOut)
{
   check(!bTimedOut);
   check(ReceiveDoublePress);
   HandleDoublePress();
}

void UTATGameplayAbility_ReceiveHoldAndDoublePress::_OnDoublePressTimeOut(float timeHeld, bool bTimedOut)
{
   check(bTimedOut);
   check(ReceiveDoublePress);
   HandleSinglePress();
}

void UTATGameplayAbility_ReceiveHoldAndDoublePress::_OnInitialPressTimeOut(float timeHeld, bool bTimedOut)
{
   check(bTimedOut);
   if (ReceivePressAndHold)
   {
      HandlePressAndHold();
   }
   else
   {
      HandleSinglePress();
   }
}
